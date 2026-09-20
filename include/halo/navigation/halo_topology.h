#pragma once

#include "../core/halo_memory.h"
#include "../utils/halo_heap.h"
#include "../utils/halo_math.h"
#include "../utils/halo_types.h"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstring>

namespace halo::topology {

// ============================================================================
// 1. HEXAGONAL AXIAL/CUBE PATHFINDING (CIVILIZATION / WARGAME TOPOLOGY)
// ============================================================================

struct HexPathResult {
  bool found = false;
  int32_t len = 0;
  HexCoord route[Config::MAX_PATH_LEN];
};

template <int32_t Q_MAX = 128, int32_t R_MAX = 128>
class alignas(64) HexGridMap {
private:
  static constexpr int32_t TOTAL_CELLS = Q_MAX * R_MAX;
  uint8_t *m_walkable = nullptr;
  int32_t m_qMax = Q_MAX;
  int32_t m_rMax = R_MAX;

public:
  HexGridMap() noexcept = default;

  void Init(int32_t qMax, int32_t rMax, memory::ArenaAllocator &arena) noexcept {
    m_qMax = qMax;
    m_rMax = rMax;
    m_walkable = arena.AllocateArray<uint8_t, 64>(static_cast<size_t>(qMax) * rMax);
    if (m_walkable) {
      std::memset(m_walkable, 1, static_cast<size_t>(qMax) * rMax);
    }
  }

  [[nodiscard]] HALO_INLINE bool InBounds(HexCoord c) const noexcept { return c.q >= 0 && c.q < m_qMax && c.r >= 0 && c.r < m_rMax; }

  [[nodiscard]] HALO_INLINE int32_t ToIndex(HexCoord c) const noexcept { return c.r * m_qMax + c.q; }

  [[nodiscard]] HALO_INLINE HexCoord ToCoord(int32_t idx) const noexcept { return HexCoord{idx % m_qMax, idx / m_qMax}; }

  [[nodiscard]] HALO_INLINE bool IsWalkable(HexCoord c) const noexcept {
    if (HALO_UNLIKELY(!InBounds(c) || !m_walkable)) return false;
    return m_walkable[ToIndex(c)] != 0;
  }

  HALO_INLINE void SetWalkable(HexCoord c, bool walkable) noexcept {
    if (HALO_LIKELY(InBounds(c) && m_walkable)) {
      m_walkable[ToIndex(c)] = walkable ? 1 : 0;
    }
  }

  [[nodiscard]] inline int32_t QMax() const noexcept { return m_qMax; }
  [[nodiscard]] inline int32_t RMax() const noexcept { return m_rMax; }
  [[nodiscard]] inline int32_t TotalCells() const noexcept { return m_qMax * m_rMax; }
};

template <int32_t Q_MAX = 128, int32_t R_MAX = 128>
class alignas(64) HexPathfinder {
private:
  PathNode *m_nodes = nullptr;
  MinHeap4Way m_heap;
  uint32_t m_epoch = 0;

public:
  HexPathfinder() noexcept = default;

  void Init(memory::ArenaAllocator &arena) noexcept {
    constexpr int32_t cap = Q_MAX * R_MAX;
    m_nodes = arena.AllocateArray<PathNode, 64>(cap);
    if (m_nodes) {
      std::memset(m_nodes, 0, cap * sizeof(PathNode));
    }
    m_heap.Init(cap, m_nodes, arena);
  }

  [[gnu::cold]] [[gnu::noinline]] HexPathResult FindPath(const HexGridMap<Q_MAX, R_MAX> &grid, HexCoord start, HexCoord goal) noexcept {
    HexPathResult res;
    if (!grid.IsWalkable(start) || !grid.IsWalkable(goal)) return res;
    if (start == goal) {
      res.found = true;
      res.len = 1;
      res.route[0] = start;
      return res;
    }

    ++m_epoch;
    m_heap.Clear();

    int32_t startIdx = grid.ToIndex(start);
    int32_t goalIdx = grid.ToIndex(goal);

    PathNode &sNode = m_nodes[startIdx];
    sNode.searchEpoch = m_epoch;
    sNode.g = 0;
    sNode.h = start.DistanceTo(goal) * Config::FP_MULT;
    sNode.f = sNode.h;
    sNode.parent = -1;
    sNode.index = startIdx;
    sNode.state = 1;

    m_heap.Push(startIdx);

    while (!m_heap.Empty()) {
      int32_t cIdx = m_heap.Pop();
      PathNode &cNode = m_nodes[cIdx];
      cNode.state = 2;

      if (cIdx == goalIdx) {
        res.found = true;
        int32_t curr = goalIdx;
        while (curr != -1 && res.len < Config::MAX_PATH_LEN) {
          res.route[res.len++] = grid.ToCoord(curr);
          curr = m_nodes[curr].parent;
        }
        for (int32_t i = 0; i < res.len / 2; ++i) {
          std::swap(res.route[i], res.route[res.len - 1 - i]);
        }
        return res;
      }

      HexCoord cCoord = grid.ToCoord(cIdx);
      for (int32_t d = 0; d < 6; ++d) {
        HexCoord nCoord = cCoord.Neighbor(d);
        if (!grid.IsWalkable(nCoord)) continue;

        int32_t nIdx = grid.ToIndex(nCoord);
        PathNode &nNode = m_nodes[nIdx];
        if (nNode.searchEpoch != m_epoch) {
          nNode.searchEpoch = m_epoch;
          nNode.state = 0;
          nNode.g = 0x3FFFFFFF;
          nNode.parent = -1;
          nNode.index = nIdx;
        }
        if (nNode.state == 2) continue;

        int32_t g = cNode.g + Config::FP_MULT;
        if (g < nNode.g) {
          nNode.g = g;
          nNode.h = nCoord.DistanceTo(goal) * Config::FP_MULT;
          nNode.f = nNode.g + nNode.h;
          nNode.parent = cIdx;

          if (nNode.state == 0) {
            nNode.state = 1;
            m_heap.Push(nIdx);
          } else {
            m_heap.DecreaseKey(nIdx);
          }
        }
      }
    }
    return res;
  }
};

// ============================================================================
// 2. LAYERED 2.5D MULTI-FLOOR MESH (STAIRS, BRIDGES, RAMPS, ELEVATORS)
// ============================================================================

struct VerticalLink {
  FloorCoord from;
  FloorCoord to;
  int32_t costFP = Config::FP_MULT * 2;  // Extra transition penalty for stairs
};

struct MultiFloorPathResult {
  bool found = false;
  int32_t len = 0;
  FloorCoord route[Config::MAX_PATH_LEN];
};

template <int32_t W = 128, int32_t H = 128, int32_t MAX_FLOORS = 4>
class alignas(64) MultiFloorGraph {
private:
  static constexpr int32_t CELLS_PER_FLOOR = W * H;
  static constexpr int32_t TOTAL_CELLS = CELLS_PER_FLOOR * MAX_FLOORS;
  static constexpr int32_t MAX_LINKS = 64;

  uint8_t *m_walkable = nullptr;
  VerticalLink m_links[MAX_LINKS];
  int32_t m_linkCount = 0;
  int32_t m_floors = MAX_FLOORS;

public:
  MultiFloorGraph() noexcept = default;

  void Init(int32_t floors, memory::ArenaAllocator &arena) noexcept {
    m_floors = std::min(floors, MAX_FLOORS);
    m_walkable = arena.AllocateArray<uint8_t, 64>(static_cast<size_t>(TOTAL_CELLS));
    if (m_walkable) {
      std::memset(m_walkable, 1, static_cast<size_t>(TOTAL_CELLS));
    }
    m_linkCount = 0;
  }

  [[nodiscard]] HALO_INLINE int32_t ToIndex(FloorCoord c) const noexcept { return c.floor * CELLS_PER_FLOOR + c.y * W + c.x; }

  [[nodiscard]] HALO_INLINE FloorCoord ToCoord(int32_t idx) const noexcept {
    int32_t floor = idx / CELLS_PER_FLOOR;
    int32_t rem = idx % CELLS_PER_FLOOR;
    return FloorCoord{rem % W, rem / W, floor};
  }

  [[nodiscard]] HALO_INLINE bool InBounds(FloorCoord c) const noexcept {
    return c.x >= 0 && c.x < W && c.y >= 0 && c.y < H && c.floor >= 0 && c.floor < m_floors;
  }

  [[nodiscard]] HALO_INLINE bool IsWalkable(FloorCoord c) const noexcept {
    if (HALO_UNLIKELY(!InBounds(c) || !m_walkable)) return false;
    return m_walkable[ToIndex(c)] != 0;
  }

  HALO_INLINE void SetWalkable(FloorCoord c, bool walkable) noexcept {
    if (HALO_LIKELY(InBounds(c) && m_walkable)) {
      m_walkable[ToIndex(c)] = walkable ? 1 : 0;
    }
  }

  bool AddVerticalLink(FloorCoord from, FloorCoord to, int32_t costFP = Config::FP_MULT * 2) noexcept {
    if (m_linkCount >= MAX_LINKS || !InBounds(from) || !InBounds(to)) return false;
    m_links[m_linkCount++] = VerticalLink{from, to, costFP};
    return true;
  }

  [[nodiscard]] inline int32_t GetLinkCount() const noexcept { return m_linkCount; }
  [[nodiscard]] inline const VerticalLink *GetLinks() const noexcept { return m_links; }
  [[nodiscard]] inline int32_t TotalNodes() const noexcept { return m_floors * CELLS_PER_FLOOR; }
};

template <int32_t W = 128, int32_t H = 128, int32_t MAX_FLOORS = 4>
class alignas(64) MultiFloorPathfinder {
private:
  PathNode *m_nodes = nullptr;
  MinHeap4Way m_heap;
  uint32_t m_epoch = 0;

public:
  MultiFloorPathfinder() noexcept = default;

  void Init(memory::ArenaAllocator &arena) noexcept {
    constexpr int32_t cap = W * H * MAX_FLOORS;
    m_nodes = arena.AllocateArray<PathNode, 64>(cap);
    if (m_nodes) {
      std::memset(m_nodes, 0, cap * sizeof(PathNode));
    }
    m_heap.Init(cap, m_nodes, arena);
  }

  [[gnu::cold]] [[gnu::noinline]] MultiFloorPathResult FindPath(const MultiFloorGraph<W, H, MAX_FLOORS> &graph, FloorCoord start,
                                                                FloorCoord goal) noexcept {
    MultiFloorPathResult res;
    if (!graph.IsWalkable(start) || !graph.IsWalkable(goal)) return res;
    if (start == goal) {
      res.found = true;
      res.len = 1;
      res.route[0] = start;
      return res;
    }

    ++m_epoch;
    m_heap.Clear();

    int32_t startIdx = graph.ToIndex(start);
    int32_t goalIdx = graph.ToIndex(goal);

    PathNode &sNode = m_nodes[startIdx];
    sNode.searchEpoch = m_epoch;
    sNode.g = 0;
    int32_t dx = std::abs(start.x - goal.x);
    int32_t dy = std::abs(start.y - goal.y);
    int32_t df = std::abs(start.floor - goal.floor);
    sNode.h = (std::max(dx, dy) * Config::FP_MULT) + (df * Config::FP_MULT * 3);
    sNode.f = sNode.h;
    sNode.parent = -1;
    sNode.index = startIdx;
    sNode.state = 1;

    m_heap.Push(startIdx);

    auto RelaxNode = [&](int32_t cIdx, int32_t nIdx, int32_t stepCost, FloorCoord nCoord) {
      PathNode &nNode = m_nodes[nIdx];
      if (nNode.searchEpoch != m_epoch) {
        nNode.searchEpoch = m_epoch;
        nNode.state = 0;
        nNode.g = 0x3FFFFFFF;
        nNode.parent = -1;
        nNode.index = nIdx;
      }
      if (nNode.state == 2) return;

      int32_t g = m_nodes[cIdx].g + stepCost;
      if (g < nNode.g) {
        nNode.g = g;
        int32_t ndx = std::abs(nCoord.x - goal.x);
        int32_t ndy = std::abs(nCoord.y - goal.y);
        int32_t ndf = std::abs(nCoord.floor - goal.floor);
        nNode.h = (std::max(ndx, ndy) * Config::FP_MULT) + (ndf * Config::FP_MULT * 3);
        nNode.f = nNode.g + nNode.h;
        nNode.parent = cIdx;

        if (nNode.state == 0) {
          nNode.state = 1;
          m_heap.Push(nIdx);
        } else {
          m_heap.DecreaseKey(nIdx);
        }
      }
    };

    while (!m_heap.Empty()) {
      int32_t cIdx = m_heap.Pop();
      PathNode &cNode = m_nodes[cIdx];
      cNode.state = 2;

      if (cIdx == goalIdx) {
        res.found = true;
        int32_t curr = goalIdx;
        while (curr != -1 && res.len < Config::MAX_PATH_LEN) {
          res.route[res.len++] = graph.ToCoord(curr);
          curr = m_nodes[curr].parent;
        }
        for (int32_t i = 0; i < res.len / 2; ++i) {
          std::swap(res.route[i], res.route[res.len - 1 - i]);
        }
        return res;
      }

      FloorCoord cCoord = graph.ToCoord(cIdx);

      // 1. Planar 8-way movement on current floor
      for (int32_t i = 0; i < 8; ++i) {
        FloorCoord nCoord{cCoord.x + Direction::Offsets[i].x, cCoord.y + Direction::Offsets[i].y, cCoord.floor};
        if (graph.IsWalkable(nCoord)) {
          RelaxNode(cIdx, graph.ToIndex(nCoord), Direction::CostFP[i], nCoord);
        }
      }

      // 2. Vertical transition links (stairs, elevators, bridges)
      const int32_t numLinks = graph.GetLinkCount();
      const VerticalLink *links = graph.GetLinks();
      for (int32_t l = 0; l < numLinks; ++l) {
        FloorCoord target = (links[l].from == cCoord) ? links[l].to : (links[l].to == cCoord) ? links[l].from : FloorCoord{-1, -1, -1};
        if (target.x >= 0 && graph.IsWalkable(target)) {
          RelaxNode(cIdx, graph.ToIndex(target), links[l].costFP, target);
        }
      }
    }
    return res;
  }
};

// ============================================================================
// 3. 3D VOXEL COORDINATE MAPS & FAST 3D DDA RAYCASTING
// ============================================================================

template <int32_t VX = 64, int32_t VY = 64, int32_t VZ = 32>
class alignas(64) VoxelGrid3D {
private:
  static constexpr int32_t TOTAL_VOXELS = VX * VY * VZ;
  static constexpr int32_t WORDS_TOTAL = (TOTAL_VOXELS + 63) / 64;

  uint64_t *m_bits = nullptr;

public:
  VoxelGrid3D() noexcept = default;

  void Init(memory::ArenaAllocator &arena) noexcept {
    m_bits = arena.AllocateArray<uint64_t, 64>(WORDS_TOTAL);
    if (m_bits) {
      std::memset(m_bits, 0, WORDS_TOTAL * sizeof(uint64_t));
    }
  }

  [[nodiscard]] HALO_INLINE bool InBounds(int32_t x, int32_t y, int32_t z) const noexcept {
    return x >= 0 && x < VX && y >= 0 && y < VY && z >= 0 && z < VZ;
  }

  [[nodiscard]] HALO_INLINE bool InBounds(VoxelCoord v) const noexcept { return InBounds(v.x, v.y, v.z); }

  [[nodiscard]] HALO_INLINE int32_t ToIndex(int32_t x, int32_t y, int32_t z) const noexcept { return (z * VY + y) * VX + x; }

  [[nodiscard]] HALO_INLINE bool IsVoxelSolid(int32_t x, int32_t y, int32_t z) const noexcept {
    if (HALO_UNLIKELY(!InBounds(x, y, z) || !m_bits)) return true;
    int32_t idx = ToIndex(x, y, z);
    return (m_bits[idx >> 6] & (1ULL << (idx & 63))) != 0;
  }

  [[nodiscard]] HALO_INLINE bool IsVoxelSolid(VoxelCoord v) const noexcept { return IsVoxelSolid(v.x, v.y, v.z); }

  HALO_INLINE void SetSolid(int32_t x, int32_t y, int32_t z, bool solid = true) noexcept {
    if (HALO_LIKELY(InBounds(x, y, z) && m_bits)) {
      int32_t idx = ToIndex(x, y, z);
      if (solid) {
        m_bits[idx >> 6] |= (1ULL << (idx & 63));
      } else {
        m_bits[idx >> 6] &= ~(1ULL << (idx & 63));
      }
    }
  }

  // 3D DDA (Digital Differential Analyzer) Voxel Line Raycast
  // Returns true if ray from start to end is unobstructed; false if hit solid voxel
  [[gnu::cold]] [[gnu::noinline]] [[nodiscard]] bool RaycastDDA(Vec3f start, Vec3f end, Vec3f *outHitPos = nullptr) const noexcept {
    Vec3f dir = end - start;
    float maxDist = dir.Length();
    if (maxDist < 1e-5f) return !IsVoxelSolid(static_cast<int32_t>(start.x), static_cast<int32_t>(start.y), static_cast<int32_t>(start.z));

    dir = dir / maxDist;

    int32_t stepX = (dir.x > 0.0f) ? 1 : ((dir.x < 0.0f) ? -1 : 0);
    int32_t stepY = (dir.y > 0.0f) ? 1 : ((dir.y < 0.0f) ? -1 : 0);
    int32_t stepZ = (dir.z > 0.0f) ? 1 : ((dir.z < 0.0f) ? -1 : 0);

    float tDeltaX = (stepX != 0) ? std::abs(1.0f / dir.x) : 1e9f;
    float tDeltaY = (stepY != 0) ? std::abs(1.0f / dir.y) : 1e9f;
    float tDeltaZ = (stepZ != 0) ? std::abs(1.0f / dir.z) : 1e9f;

    int32_t curX = static_cast<int32_t>(std::floor(start.x));
    int32_t curY = static_cast<int32_t>(std::floor(start.y));
    int32_t curZ = static_cast<int32_t>(std::floor(start.z));

    float tMaxX = (stepX > 0) ? (static_cast<float>(curX + 1) - start.x) * tDeltaX : (start.x - static_cast<float>(curX)) * tDeltaX;
    float tMaxY = (stepY > 0) ? (static_cast<float>(curY + 1) - start.y) * tDeltaY : (start.y - static_cast<float>(curY)) * tDeltaY;
    float tMaxZ = (stepZ > 0) ? (static_cast<float>(curZ + 1) - start.z) * tDeltaZ : (start.z - static_cast<float>(curZ)) * tDeltaZ;

    float tCurrent = 0.0f;

    while (tCurrent <= maxDist) {
      if (IsVoxelSolid(curX, curY, curZ)) {
        if (outHitPos) {
          *outHitPos = start + dir * tCurrent;
        }
        return false;  // Hit solid
      }

      if (tMaxX < tMaxY) {
        if (tMaxX < tMaxZ) {
          tCurrent = tMaxX;
          tMaxX += tDeltaX;
          curX += stepX;
        } else {
          tCurrent = tMaxZ;
          tMaxZ += tDeltaZ;
          curZ += stepZ;
        }
      } else {
        if (tMaxY < tMaxZ) {
          tCurrent = tMaxY;
          tMaxY += tDeltaY;
          curY += stepY;
        } else {
          tCurrent = tMaxZ;
          tMaxZ += tDeltaZ;
          curZ += stepZ;
        }
      }
    }
    return true;  // Clear line of sight
  }
};

}  // namespace halo::topology
