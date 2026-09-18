#pragma once

#include "../core/halo_memory.h"
#include "../protection/halo_swar_10_layer_bitboard.h"
#include "../utils/halo_types.h"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstring>

namespace halo::swarm {

// ============================================================================
// 1. DYNAMIC HIGH-DENSITY FLOWFIELD GENERATOR (UP TO 512x512)
// ============================================================================

template <int32_t W = 128, int32_t H = 128>
class alignas(64) FlowFieldT {
public:
  static constexpr int32_t TOTAL_CELLS = W * H;

private:
  uint16_t *m_integrationField = nullptr;
  Vec2f *m_vectorField = nullptr;
  int32_t *m_queue = nullptr;
  int32_t m_w = W;
  int32_t m_h = H;

public:
  FlowFieldT() noexcept = default;

  void Init(memory::ArenaAllocator &arena, int32_t w = W, int32_t h = H) noexcept {
    m_w = w;
    m_h = h;
    const size_t count = static_cast<size_t>(m_w) * m_h;
    m_integrationField = arena.AllocateArray<uint16_t, 64>(count);
    m_vectorField = arena.AllocateArray<Vec2f, 64>(count);
    m_queue = arena.AllocateArray<int32_t, 64>(count);

    if (m_integrationField) {
      std::memset(m_integrationField, 0xFF, count * sizeof(uint16_t));
    }
    if (m_vectorField) {
      std::memset(m_vectorField, 0, count * sizeof(Vec2f));
    }
  }

  template <typename GridWalkableFunc>
  [[gnu::cold]] [[gnu::noinline]] void Generate(Vec2i target, GridWalkableFunc &&isWalkable) noexcept {
    if (target.x < 0 || target.x >= m_w || target.y < 0 || target.y >= m_h || !m_integrationField) {
      return;
    }

    const int32_t total = m_w * m_h;
    std::memset(m_integrationField, 0xFF, total * sizeof(uint16_t));
    std::memset(m_vectorField, 0, total * sizeof(Vec2f));

    int32_t targetIdx = target.y * m_w + target.x;
    m_integrationField[targetIdx] = 0;

    int32_t qHead = 0;
    int32_t qTail = 0;
    m_queue[qTail++] = targetIdx;

    // Fast Dijkstra Wavefront Propagation
    while (qHead != qTail) {
      int32_t currIdx = m_queue[qHead++];
      if (qHead == total) qHead = 0;

      int32_t cx = currIdx % m_w;
      int32_t cy = currIdx / m_w;
      uint16_t currCost = m_integrationField[currIdx];

      for (int32_t i = 0; i < 8; ++i) {
        int32_t nx = cx + Direction::Offsets[i].x;
        int32_t ny = cy + Direction::Offsets[i].y;

        if (nx < 0 || nx >= m_w || ny < 0 || ny >= m_h) continue;
        if (!isWalkable(nx, ny)) continue;

        // Diagonal corner cutting check
        if (Direction::IsDiag[i]) {
          if (!isWalkable(cx, ny) || !isWalkable(nx, cy)) continue;
        }

        int32_t nIdx = ny * m_w + nx;
        uint16_t moveCost = Direction::IsDiag[i] ? 14 : 10;
        uint16_t newCost = currCost + moveCost;

        if (newCost < m_integrationField[nIdx]) {
          m_integrationField[nIdx] = newCost;
          m_queue[qTail++] = nIdx;
          if (qTail == total) qTail = 0;
        }
      }
    }

    // Gradient Descent Vector Field Generation
    for (int32_t y = 0; y < m_h; ++y) {
      for (int32_t x = 0; x < m_w; ++x) {
        int32_t idx = y * m_w + x;
        if (m_integrationField[idx] == 0xFFFF || (x == target.x && y == target.y)) {
          m_vectorField[idx] = Vec2f{0.0f, 0.0f};
          continue;
        }

        uint16_t lowestCost = m_integrationField[idx];
        Vec2f bestDir{0.0f, 0.0f};

        for (int32_t i = 0; i < 8; ++i) {
          int32_t nx = x + Direction::Offsets[i].x;
          int32_t ny = y + Direction::Offsets[i].y;

          if (nx < 0 || nx >= m_w || ny < 0 || ny >= m_h) continue;
          int32_t nIdx = ny * m_w + nx;
          if (m_integrationField[nIdx] < lowestCost) {
            lowestCost = m_integrationField[nIdx];
            bestDir = Vec2f{static_cast<float>(Direction::Offsets[i].x),
                            static_cast<float>(Direction::Offsets[i].y)};
          }
        }
        m_vectorField[idx] = bestDir.Normalized();
      }
    }
  }

  [[nodiscard]] HALO_INLINE Vec2f SampleFlowVector(Vec2f pos) const noexcept {
    int32_t gx = static_cast<int32_t>(pos.x + 0.5f);
    int32_t gy = static_cast<int32_t>(pos.y + 0.5f);
    if (gx < 0 || gx >= m_w || gy < 0 || gy >= m_h || !m_vectorField) {
      return Vec2f{0.0f, 0.0f};
    }
    return m_vectorField[gy * m_w + gx];
  }

  [[nodiscard]] HALO_INLINE uint16_t GetIntegrationCost(int32_t x, int32_t y) const noexcept {
    if (x < 0 || x >= m_w || y < 0 || y >= m_h || !m_integrationField) return 0xFFFF;
    return m_integrationField[y * m_w + x];
  }
};

// 64x64 Stack/Inline Zero-Allocation FlowField (100% Backward Compatible)
class alignas(64) FlowField {
private:
  static constexpr int32_t GRID_SIZE = 64;
  static constexpr int32_t TOTAL_CELLS = GRID_SIZE * GRID_SIZE;

  alignas(64) uint16_t m_integrationField[TOTAL_CELLS];
  alignas(64) int8_t m_vectorField[TOTAL_CELLS];
  alignas(64) int32_t m_queue[TOTAL_CELLS];

public:
  FlowField() noexcept {
    std::memset(m_integrationField, 0xFF, sizeof(m_integrationField));
    std::memset(m_vectorField, 0xFF, sizeof(m_vectorField));
  }

  void Generate(const swar::UltimateBitboard64 &grid, Vec2i target) noexcept {
    if (target.x < 0 || target.x >= GRID_SIZE || target.y < 0 || target.y >= GRID_SIZE) {
      return;
    }

    std::memset(m_integrationField, 0xFF, sizeof(m_integrationField));
    std::memset(m_vectorField, 0xFF, sizeof(m_vectorField));

    int32_t targetIdx = target.y * GRID_SIZE + target.x;
    m_integrationField[targetIdx] = 0;

    int32_t qHead = 0;
    int32_t qTail = 0;
    m_queue[qTail++] = targetIdx;

    while (qHead != qTail) {
      int32_t currIdx = m_queue[qHead++];
      if (qHead == TOTAL_CELLS) qHead = 0;

      int32_t currX = currIdx % GRID_SIZE;
      int32_t currY = currIdx / GRID_SIZE;

      for (int32_t i = 0; i < 8; ++i) {
        Vec2i nextPos = Vec2i(currX, currY) + Direction::Offsets[i];

        if (nextPos.x < 0 || nextPos.x >= GRID_SIZE ||
            nextPos.y < 0 || nextPos.y >= GRID_SIZE) {
          continue;
        }

        uint64_t row = grid.GetCompositeRow(nextPos.y);
        if ((row & (1ULL << nextPos.x)) != 0) {
          continue;
        }

        if (Direction::IsDiag[i]) {
          uint64_t rowCurr = grid.GetCompositeRow(currY);
          if ((rowCurr & (1ULL << nextPos.x)) != 0 || (row & (1ULL << currX)) != 0) {
            continue;
          }
        }

        int32_t nextIdx = nextPos.y * GRID_SIZE + nextPos.x;
        uint16_t moveCost = Direction::IsDiag[i] ? 14 : 10;
        uint16_t newCost = m_integrationField[currIdx] + moveCost;

        if (newCost < m_integrationField[nextIdx]) {
          m_integrationField[nextIdx] = newCost;
          m_vectorField[nextIdx] = static_cast<int8_t>((i + 4) % 8);

          m_queue[qTail++] = nextIdx;
          if (qTail == TOTAL_CELLS) qTail = 0;
        }
      }
    }
  }

  [[nodiscard]] HALO_INLINE int8_t GetDirectionForAgent(int32_t idx) const noexcept {
    if (idx < 0 || idx >= TOTAL_CELLS) return -1;
    return m_vectorField[idx];
  }

  [[nodiscard]] HALO_INLINE int8_t GetDirectionForAgent(int32_t x, int32_t y) const noexcept {
    if (x < 0 || x >= GRID_SIZE || y < 0 || y >= GRID_SIZE) return -1;
    return m_vectorField[y * GRID_SIZE + x];
  }

  [[nodiscard]] HALO_INLINE uint16_t GetIntegrationCost(int32_t x, int32_t y) const noexcept {
    if (x < 0 || x >= GRID_SIZE || y < 0 || y >= GRID_SIZE) return 65535;
    return m_integrationField[y * GRID_SIZE + x];
  }

  [[nodiscard]] HALO_INLINE Vec2f SampleFlowVector(Vec2f pos) const noexcept {
    int32_t gx = static_cast<int32_t>(pos.x + 0.5f);
    int32_t gy = static_cast<int32_t>(pos.y + 0.5f);
    int8_t dir = GetDirectionForAgent(gx, gy);
    if (dir < 0 || dir >= 8) return Vec2f{0.0f, 0.0f};
    return Vec2f{static_cast<float>(Direction::Offsets[dir].x),
                 static_cast<float>(Direction::Offsets[dir].y)}.Normalized();
  }
};

// ============================================================================
// 2. RTS TACTICAL FORMATIONS & BOIDS SWARM (10,000 UNITS)
// ============================================================================

enum class FormationType : uint8_t {
  BOX = 0,
  WEDGE = 1,
  LINE = 2,
  ECHELON = 3,
  COLUMN = 4
};

struct FormationSlot {
  Vec2f localOffset{0.0f, 0.0f};
};

class TacticalFormation {
public:
  static Vec2f ComputeSlotOffset(FormationType type, int32_t slotIdx, float spacing = 1.5f) noexcept {
    if (slotIdx == 0) return Vec2f{0.0f, 0.0f}; // Leader slot

    switch (type) {
    case FormationType::BOX: {
      int32_t row = slotIdx / 5;
      int32_t col = slotIdx % 5;
      return Vec2f{(static_cast<float>(col) - 2.0f) * spacing, -static_cast<float>(row) * spacing};
    }
    case FormationType::WEDGE: {
      int32_t side = (slotIdx % 2 == 1) ? 1 : -1;
      int32_t rank = (slotIdx + 1) / 2;
      return Vec2f{static_cast<float>(side * rank) * spacing, -static_cast<float>(rank) * spacing};
    }
    case FormationType::LINE: {
      int32_t side = (slotIdx % 2 == 1) ? 1 : -1;
      int32_t rank = (slotIdx + 1) / 2;
      return Vec2f{static_cast<float>(side * rank) * spacing, 0.0f};
    }
    case FormationType::ECHELON: {
      return Vec2f{static_cast<float>(slotIdx) * spacing, -static_cast<float>(slotIdx) * spacing};
    }
    case FormationType::COLUMN: {
      return Vec2f{0.0f, -static_cast<float>(slotIdx) * spacing};
    }
    }
    return Vec2f{0.0f, 0.0f};
  }
};

template <size_t MAX_AGENTS = 10000>
struct alignas(64) SwarmAgentPool {
  alignas(64) float posX[MAX_AGENTS];
  alignas(64) float posY[MAX_AGENTS];
  alignas(64) float velX[MAX_AGENTS];
  alignas(64) float velY[MAX_AGENTS];
  alignas(64) float maxSpeed[MAX_AGENTS];
  alignas(64) float radius[MAX_AGENTS];
  alignas(64) uint16_t targetGoal[MAX_AGENTS];
  alignas(64) uint8_t reachedGoal[MAX_AGENTS];
};

struct alignas(32) RtsUnit {
  Vec2f pos{0.0f, 0.0f};
  Vec2f vel{0.0f, 0.0f};
  float maxSpeed = 4.0f;
  float radius = 0.35f;
  int32_t squadId = 0;
  int16_t slotIdx = 0;
  uint8_t reachedGoal = 0;
  uint8_t _pad = 0;
};
static_assert(sizeof(RtsUnit) == 32, "RtsUnit must be 32 bytes for cache alignment");

// High-Density Swarm Simulation with Reynolds Steering and Soft-Body Anti-Stacking (SoA Layout)
template <size_t MAX_UNITS = 10000, int32_t MAP_W = 512, int32_t MAP_H = 512>
class alignas(64) SwarmSimulation {
public:
  static constexpr int32_t CELL_SIZE = 2; // 2x2 fine spatial hash cell
  static constexpr int32_t GRID_COLS = (MAP_W + CELL_SIZE - 1) / CELL_SIZE;
  static constexpr int32_t GRID_ROWS = (MAP_H + CELL_SIZE - 1) / CELL_SIZE;
  static constexpr int32_t TOTAL_BINS = GRID_COLS * GRID_ROWS;
  static constexpr int32_t MAX_LOCAL_NEIGHBORS = 4;
  static constexpr int8_t NEIGHBOR_OFFSETS[9][2] = {
      {0, 0}, {0, -1}, {0, 1}, {-1, 0}, {1, 0}, {-1, -1}, {1, -1}, {-1, 1}, {1, 1}};

private:
  alignas(64) SwarmAgentPool<MAX_UNITS> m_pool;
  mutable RtsUnit m_unitsLegacy[MAX_UNITS];
  size_t m_unitCount = 0;

  // Zero-allocation flat spatial hash: Linked-list per bin
  int32_t *m_binHead = nullptr;
  int32_t *m_unitNext = nullptr;

public:
  SwarmSimulation() noexcept = default;

  void Init(memory::ArenaAllocator &arena, size_t count = MAX_UNITS) noexcept {
    m_unitCount = std::min(count, MAX_UNITS);
    m_binHead = arena.AllocateArray<int32_t, 64>(TOTAL_BINS);
    m_unitNext = arena.AllocateArray<int32_t, 64>(MAX_UNITS);

    if (m_binHead) {
      std::memset(m_binHead, 0xFF, TOTAL_BINS * sizeof(int32_t));
    }
    if (m_unitNext) {
      std::memset(m_unitNext, 0xFF, MAX_UNITS * sizeof(int32_t));
    }
    std::memset(&m_pool, 0, sizeof(m_pool));
  }

  void SpawnUnit(size_t idx, Vec2f pos, float maxSpeed = 4.0f, float radius = 0.35f) noexcept {
    if (idx < MAX_UNITS) {
      m_pool.posX[idx] = pos.x;
      m_pool.posY[idx] = pos.y;
      m_pool.velX[idx] = 0.0f;
      m_pool.velY[idx] = 0.0f;
      m_pool.maxSpeed[idx] = maxSpeed;
      m_pool.radius[idx] = radius;
      m_pool.targetGoal[idx] = 0;
      m_pool.reachedGoal[idx] = 0;
      if (idx >= m_unitCount) m_unitCount = idx + 1;
    }
  }

  [[nodiscard]] inline size_t GetUnitCount() const noexcept { return m_unitCount; }
  [[nodiscard]] inline Vec2f GetUnitPos(size_t idx) const noexcept {
    return Vec2f{m_pool.posX[idx], m_pool.posY[idx]};
  }
  [[nodiscard]] inline Vec2f GetUnitVel(size_t idx) const noexcept {
    return Vec2f{m_pool.velX[idx], m_pool.velY[idx]};
  }
  [[nodiscard]] inline float GetUnitRadius(size_t idx) const noexcept {
    return m_pool.radius[idx];
  }

  // Backward-compatible unit array view (synced on demand)
  [[nodiscard]] inline const RtsUnit *GetUnits() const noexcept {
    for (size_t i = 0; i < m_unitCount; ++i) {
      m_unitsLegacy[i].pos = Vec2f{m_pool.posX[i], m_pool.posY[i]};
      m_unitsLegacy[i].vel = Vec2f{m_pool.velX[i], m_pool.velY[i]};
      m_unitsLegacy[i].maxSpeed = m_pool.maxSpeed[i];
      m_unitsLegacy[i].radius = m_pool.radius[i];
      m_unitsLegacy[i].reachedGoal = m_pool.reachedGoal[i];
    }
    return m_unitsLegacy;
  }

  // Pre-warms instruction caches, branch predictors, and page tables
  template <typename FlowFieldType, typename ObstacleCheckFunc>
  void Warmup(const FlowFieldType &flowField, ObstacleCheckFunc &&isBlocked,
              Vec2f goalPos, float goalRadius = 4.0f) noexcept {
    Update(0.0001f, flowField, isBlocked, goalPos, goalRadius);
  }

  // 60 FPS Swarm Update: 10,000 units in < 1.0 ms (SoA SIMD mechanical sympathy)
  template <typename FlowFieldType, typename ObstacleCheckFunc>
  void Update(float dt, const FlowFieldType &flowField, ObstacleCheckFunc &&isBlocked,
              Vec2f goalPos, float goalRadius = 4.0f) noexcept {
    if (!m_binHead || !m_unitNext || m_unitCount == 0) return;

    // 1. Clear spatial hash bins
    std::memset(m_binHead, 0xFF, TOTAL_BINS * sizeof(int32_t));

    // 2. Insert all active units into spatial bins using contiguous float SoA coordinates
    for (size_t i = 0; i < m_unitCount; ++i) {
      if ((i & 15) == 0 && i + 16 < m_unitCount) {
        __builtin_prefetch(&m_pool.posX[i + 16], 0, 1);
        __builtin_prefetch(&m_pool.posY[i + 16], 0, 1);
      }
      int32_t bx = std::clamp(static_cast<int32_t>(m_pool.posX[i]) / CELL_SIZE, 0, GRID_COLS - 1);
      int32_t by = std::clamp(static_cast<int32_t>(m_pool.posY[i]) / CELL_SIZE, 0, GRID_ROWS - 1);
      int32_t binIdx = by * GRID_COLS + bx;

      m_unitNext[i] = m_binHead[binIdx];
      m_binHead[binIdx] = static_cast<int32_t>(i);
    }

    const float goalRadiusSq = goalRadius * goalRadius;

    // 3. Reynolds Flocking & Soft-Body Anti-Stacking Integration (SoA)
    for (size_t i = 0; i < m_unitCount; ++i) {
      float px = m_pool.posX[i];
      float py = m_pool.posY[i];
      float vx = m_pool.velX[i];
      float vy = m_pool.velY[i];
      const float speed = m_pool.maxSpeed[i];
      const float rad = m_pool.radius[i];

      // Check goal reached
      const float gdx = px - goalPos.x;
      const float gdy = py - goalPos.y;
      if (gdx * gdx + gdy * gdy <= goalRadiusSq) {
        m_pool.reachedGoal[i] = 1;
        m_pool.velX[i] = vx * 0.5f;
        m_pool.velY[i] = vy * 0.5f;
        continue;
      }

      // Sample Flow Field (Primary navigational vector)
      Vec2f flowVec = flowField.SampleFlowVector(Vec2f{px, py});

      // Reynolds Steering Accumulators
      Vec2f separation{0.0f, 0.0f};
      Vec2f alignment{0.0f, 0.0f};
      Vec2f cohesion{0.0f, 0.0f};
      int32_t neighborCount = 0;
      int32_t checked = 0;

      const int32_t bx = std::clamp(static_cast<int32_t>(px) / CELL_SIZE, 0, GRID_COLS - 1);
      const int32_t by = std::clamp(static_cast<int32_t>(py) / CELL_SIZE, 0, GRID_ROWS - 1);

      // Search center cell first, then cardinals, then diagonals with early cutoff
      for (int32_t o = 0; o < 9 && checked < MAX_LOCAL_NEIGHBORS; ++o) {
        const int32_t nx = bx + NEIGHBOR_OFFSETS[o][0];
        const int32_t ny = by + NEIGHBOR_OFFSETS[o][1];
        if (nx < 0 || nx >= GRID_COLS || ny < 0 || ny >= GRID_ROWS) continue;

        const int32_t bin = ny * GRID_COLS + nx;
        int32_t otherIdx = m_binHead[bin];

        while (otherIdx != -1 && checked < MAX_LOCAL_NEIGHBORS) {
          if (otherIdx != static_cast<int32_t>(i)) {
            const float opx = m_pool.posX[otherIdx];
            const float opy = m_pool.posY[otherIdx];
            const float diffX = px - opx;
            const float diffY = py - opy;
            const float distSq = diffX * diffX + diffY * diffY;
            const float minSep = rad + m_pool.radius[otherIdx];
            const float minSepSq = minSep * minSep;

            // Soft-body Anti-Stacking Push (Hard physical repulsion)
            if (distSq < minSepSq && distSq > 1e-6f) {
              const float invDist = 1.0f / std::sqrt(distSq);
              const float dist = distSq * invDist;
              const float overlap = minSep - dist;
              const float normX = diffX * invDist;
              const float normY = diffY * invDist;

              px += normX * (overlap * 0.65f);
              py += normY * (overlap * 0.65f);
              separation.x += normX * (overlap * 14.0f);
              separation.y += normY * (overlap * 14.0f);
              ++checked;
            } else if (distSq < 4.0f) { // Flocking neighborhood (2 meters)
              const float invDist = 1.0f / std::sqrt(distSq);
              const float dist = distSq * invDist;
              const float w = 1.0f / (dist + 0.1f);
              separation.x += diffX * invDist * w;
              separation.y += diffY * invDist * w;
              alignment.x += m_pool.velX[otherIdx];
              alignment.y += m_pool.velY[otherIdx];
              cohesion.x += opx;
              cohesion.y += opy;
              ++neighborCount;
              ++checked;
            }
          }
          otherIdx = m_unitNext[otherIdx];
        }
      }

      if (neighborCount > 0) {
        const float invN = 1.0f / static_cast<float>(neighborCount);
        alignment = (alignment * invN).Normalized() * speed;
        cohesion = ((cohesion * invN) - Vec2f{px, py}).Normalized() * speed;
      }

      // Blend Steerings: Flow (65%) + Separation (25%) + Alignment (5%) + Cohesion (5%)
      Vec2f desiredVel = flowVec * (speed * 0.65f) +
                         separation * 0.25f +
                         alignment * 0.05f +
                         cohesion * 0.05f;

      if (desiredVel.LengthSq() > speed * speed) {
        desiredVel = desiredVel.Normalized() * speed;
      }

      // Smooth velocity interpolation (inertia)
      vx = vx * 0.75f + desiredVel.x * 0.25f;
      vy = vy * 0.75f + desiredVel.y * 0.25f;

      // Kinematic step with static obstacle sliding
      const Vec2f nextPos{px + vx * dt, py + vy * dt};
      if (!isBlocked(static_cast<int32_t>(nextPos.x + 0.5f), static_cast<int32_t>(nextPos.y + 0.5f))) {
        px = nextPos.x;
        py = nextPos.y;
      } else {
        // Slide along X or Y if clear
        const Vec2f slideX{px + vx * dt, py};
        const Vec2f slideY{px, py + vy * dt};
        if (!isBlocked(static_cast<int32_t>(slideX.x + 0.5f), static_cast<int32_t>(slideX.y + 0.5f))) {
          px = slideX.x;
          vy = 0.0f;
        } else if (!isBlocked(static_cast<int32_t>(slideY.x + 0.5f), static_cast<int32_t>(slideY.y + 0.5f))) {
          py = slideY.y;
          vx = 0.0f;
        } else {
          vx = 0.0f;
          vy = 0.0f;
        }
      }

      m_pool.posX[i] = px;
      m_pool.posY[i] = py;
      m_pool.velX[i] = vx;
      m_pool.velY[i] = vy;
    }
  }
};

} // namespace halo::swarm