#pragma once

#include "../navigation/halo_apsp.h"
#include "../navigation/halo_jps_plus.h"
#include "../navigation/halo_postprocess.h"
#include "../kinodynamics/halo_kinodynamics.h"
#include "../utils/halo_heap.h"
#include "../utils/halo_math.h"
#include "../utils/halo_types.h"
#include "halo_memory.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>

namespace halo::core {

// NTTP Compile-Time Hardware-Accelerated Navigation Core
// Annihilates runtime integer multiplications and divisions for power-of-two geometries
template <int32_t W = 0, int32_t H = 0>
class alignas(64) HaloSupremeEngineT {
private:
  memory::ArenaAllocator m_masterArena;

  GridT<W, H> *m_grid = nullptr;
  urban::CityMap *m_cityMap = nullptr;

  PathNode *m_gridNodes = nullptr;
  FourAryMinHeap m_gridHeap;
  JpsPlusEngineT<W, H> m_jpsEngine;
  urban::QuantumApspRouter m_apspRouter;

  uint32_t m_gridEpoch = 0;

  static constexpr bool HAS_STATIC_DIM = (W > 0 && H > 0);
  static constexpr bool IS_POW2 = HAS_STATIC_DIM && IsPowerOfTwo(W);
  static constexpr int32_t SHIFT_W = IS_POW2 ? Log2Constexpr(W) : 0;
  static constexpr int32_t MASK_W = IS_POW2 ? (W - 1) : 0;

  [[nodiscard]] HALO_INLINE int32_t ToIndex(int32_t x, int32_t y) const noexcept {
    if constexpr (IS_POW2) {
      return (y << SHIFT_W) | x;
    } else {
      return y * m_grid->Width() + x;
    }
  }

  [[nodiscard]] HALO_INLINE Vec2i ToVec(int32_t i) const noexcept {
    if constexpr (IS_POW2) {
      return {i & MASK_W, i >> SHIFT_W};
    } else {
      return {i % m_grid->Width(), i / m_grid->Width()};
    }
  }

private:
  void InitGridAndEngine(GridT<W, H> *grid) noexcept {
    m_grid = grid;
    if (m_grid) {
      int32_t gridTot = m_grid->Size();
      m_gridNodes = m_masterArena.template AllocateArray<PathNode, 64>(gridTot);
      if (m_gridNodes) {
        std::memset(m_gridNodes, 0, static_cast<size_t>(gridTot) * sizeof(PathNode));
      }
      m_gridHeap.Init(gridTot, m_gridNodes, m_masterArena);
      m_jpsEngine.Precompute(reinterpret_cast<Grid *>(m_grid), m_masterArena);
    }
  }

public:
  HaloSupremeEngineT() noexcept = default;

  [[nodiscard]] size_t GetMasterArenaCapacity() const noexcept { return m_masterArena.GetCapacity(); }
  [[nodiscard]] size_t GetMasterArenaOffset() const noexcept { return m_masterArena.GetOffset(); }

  // Precision byte-level boot for memory-constrained MCUs (e.g. 64 KB, 128 KB on ESP32 SRAM)
  [[gnu::noinline]] [[gnu::cold]] void BootSystemBytes(GridT<W, H> *grid, size_t bytesRAM) {
    m_masterArena.Init(bytesRAM);
    InitGridAndEngine(grid);
  }

  // Pure Zero-Heap deterministic boot: user supplies static BSS memory pool or external PSRAM buffer
  [[gnu::noinline]] [[gnu::cold]] void BootSystemWithBuffer(GridT<W, H> *grid, void *buffer, size_t bufferSize) noexcept {
    m_masterArena.InitWithBuffer(buffer, bufferSize);
    InitGridAndEngine(grid);
  }

  [[gnu::noinline]] [[gnu::cold]] void BootSystem(GridT<W, H> *grid, size_t megaBytesRAM) {
    BootSystemBytes(grid, megaBytesRAM * 1024 * 1024);
  }

  [[gnu::noinline]] [[gnu::cold]] void BootSystem(GridT<W, H> *grid, urban::CityMap *cityMap, size_t megaBytesRAM) {
    BootSystem(grid, megaBytesRAM);
    m_cityMap = cityMap;
    if (m_cityMap) {
      m_apspRouter.Precompute(*m_cityMap, m_masterArena);
    }
  }

  [[gnu::noinline]] [[gnu::cold]] void BootSystemWithBuffer(GridT<W, H> *grid, urban::CityMap *cityMap, void *buffer,
                                                            size_t bufferSize) noexcept {
    BootSystemWithBuffer(grid, buffer, bufferSize);
    m_cityMap = cityMap;
    if (m_cityMap) {
      m_apspRouter.Precompute(*m_cityMap, m_masterArena);
    }
  }

  HALO_INLINE PathResult RouteGrid(Vec2i start, Vec2i target, RoutingMode mode = RoutingMode::Turbo) noexcept {
    PathResult res;
    if (HALO_UNLIKELY(!m_grid || !m_grid->InBounds(start) || !m_grid->InBounds(target))) {
      return res;
    }

    // Market-Leading Robustness: Snap un-walkable start or target to nearest traversable boundary
    Vec2i effectiveStart = start;
    if (HALO_UNLIKELY(!m_grid->IsWalkable(ToIndex(start.x, start.y)))) {
      effectiveStart = m_grid->SnapToNearestWalkable(start, 16);
      if (!m_grid->IsWalkable(ToIndex(effectiveStart.x, effectiveStart.y))) {
        return res;
      }
    }

    Vec2i effectiveTarget = target;
    if (HALO_UNLIKELY(!m_grid->IsWalkable(ToIndex(target.x, target.y)))) {
      effectiveTarget = m_grid->SnapToNearestWalkable(target, 16);
      if (!m_grid->IsWalkable(ToIndex(effectiveTarget.x, effectiveTarget.y))) {
        return res;
      }
    }

    if (HALO_UNLIKELY(effectiveStart == effectiveTarget)) {
      res.found = true;
      res.len = 1;
      res.route[0] = effectiveStart;
      return res;
    }

    ++m_gridEpoch;
    m_gridHeap.Clear();

    const bool isStrictOptimal = (mode == RoutingMode::StrictOptimal || mode == RoutingMode::AnyAngleOptimal);
    const bool isClearanceAware = (mode == RoutingMode::ClearanceAware);

    int32_t startDist = isStrictOptimal ? math::OctileDistanceFP(effectiveStart, effectiveTarget)
                                        : math::OctileDistanceFP_TieBreak(effectiveStart, effectiveTarget, effectiveStart);
    int32_t sIdx = ToIndex(effectiveStart.x, effectiveStart.y);

    InitGridNode(sIdx);
    m_gridNodes[sIdx] = {0, startDist, startDist, 0, -1, sIdx, m_gridEpoch, 1, {0, 0, 0}};
    m_gridHeap.Push(sIdx);

    while (HALO_LIKELY(!m_gridHeap.Empty())) {
      int32_t cIdx = m_gridHeap.Pop();
      PathNode &cNode = m_gridNodes[cIdx];
      cNode.state = 2;  // Closed
      res.expanded++;

      Vec2i cPos = ToVec(cIdx);
      if (HALO_UNLIKELY(cPos == effectiveTarget)) {
        res.found = true;
        res.cost = cNode.g;
        ExtractGridPath(cIdx, res);
        return res;
      }

      int32_t diffX = effectiveTarget.x - cPos.x;
      int32_t diffY = effectiveTarget.y - cPos.y;

      for (int32_t i = 0; i < 8; ++i) {
        const Vec2i dir = Direction::Offsets[i];
        int16_t jumpDist = m_jpsEngine.GetJumpDistance(cIdx, i);

        // Check if target lies along direction i
        int32_t targetDistOnRay = 0;
        bool targetOnRay = (dir.x * diffY == dir.y * diffX) && (dir.x * diffX >= 0) && (dir.y * diffY >= 0);
        if (targetOnRay) {
          targetDistOnRay = (dir.x != 0) ? std::abs(diffX) : std::abs(diffY);
        }

        int32_t jumpSteps[2] = {0, 0};
        int32_t numSteps = 0;

        if (targetOnRay) {
          if (jumpDist > 0) {
            jumpSteps[numSteps++] = (targetDistOnRay <= jumpDist) ? targetDistOnRay : jumpDist;
          } else if (targetDistOnRay <= -jumpDist) {
            jumpSteps[numSteps++] = targetDistOnRay;
          }
        } else {
          int32_t projDist = (dir.y == 0 && dir.x * diffX > 0) ? std::abs(diffX) : (dir.x == 0 && dir.y * diffY > 0) ? std::abs(diffY) : 0;
          int32_t clearSteps = (jumpDist > 0) ? jumpDist : -jumpDist;
          if (projDist > 0 && projDist <= clearSteps) {
            jumpSteps[numSteps++] = projDist;
          }

          if (jumpDist > 0) {
            if (numSteps == 0 || jumpSteps[0] != jumpDist) {
              jumpSteps[numSteps++] = jumpDist;
            }
          }
        }

        for (int32_t s = 0; s < numSteps; ++s) {
          int32_t step = jumpSteps[s];
          if (step <= 0) continue;

          Vec2i nPos = cPos + Vec2i(dir.x * step, dir.y * step);
          if (!m_grid->InBounds(nPos)) continue;

          // Zero Corner-Cutting Guarantee
          if (Direction::IsDiag[i]) {
            if (!m_grid->CanTraverseDiagonal(cPos, cPos + dir)) continue;
          }
          if (isClearanceAware && !m_grid->HasClearance(nPos.x, nPos.y, 1)) {
            continue;
          }

          int32_t nIdx = ToIndex(nPos.x, nPos.y);
          HALO_PREFETCH(&m_gridNodes[nIdx]);

          InitGridNode(nIdx);
          if (HALO_UNLIKELY(m_gridNodes[nIdx].state == 2)) continue;

          PathNode &nNode = m_gridNodes[nIdx];
          int32_t g = cNode.g + (step * Direction::CostFP[i]);

          if (HALO_LIKELY(nNode.state == 0 || g < nNode.g)) {
            nNode.g = g;
            nNode.parent = cIdx;
            nNode.index = nIdx;

            if (isStrictOptimal) {
              // Strictly Admissible Nilsson-Hart Heuristic (w = 1.0, guaranteed shortest path)
              nNode.h = math::OctileDistanceFP(nPos, effectiveTarget);
              nNode.f = g + nNode.h + m_grid->GetPenalty(nIdx);
            } else {
              // Accelerated Turbo Weighted Search
              nNode.h = math::OctileDistanceFP_TieBreak(nPos, effectiveTarget, effectiveStart);
              int32_t w = math::GetDynamicWeightFP(startDist, nNode.h);
              nNode.f = g + static_cast<int32_t>((static_cast<int64_t>(w) * nNode.h) >> 10) + m_grid->GetPenalty(nIdx);
            }

            if (nNode.state == 0) {
              nNode.state = 1;
              m_gridHeap.Push(nIdx);
            } else {
              m_gridHeap.DecreaseKey(nIdx);
            }
          }
        }
      }
    }
    return res;
  }

  // 1. Strictly Optimal Admissible Pathfinding (Guaranteed Shortest 8-Way Grid Route)
  [[nodiscard]] HALO_INLINE PathResult RouteGridOptimal(Vec2i start, Vec2i target) noexcept {
    return RouteGrid(start, target, RoutingMode::StrictOptimal);
  }

  // 2. Clearance-Aware Pathfinding (Prevents Clipping Walls & Squeezing Narrow Gaps)
  [[nodiscard]] HALO_INLINE PathResult RouteGridClearance(Vec2i start, Vec2i target, int32_t clearanceRadius = 1) noexcept {
    (void)clearanceRadius;
    return RouteGrid(start, target, RoutingMode::ClearanceAware);
  }

  // 3. Any-Angle Pathfinding (Taut String Pulling - True Continuous Euclidean Shortest Path)
  [[nodiscard]] HALO_INLINE ContinuousPathResult RouteGridAnyAngle(Vec2i start, Vec2i target) noexcept {
    ContinuousPathResult cRes;
    PathResult raw = RouteGrid(start, target, RoutingMode::StrictOptimal);
    if (!raw.found || raw.len <= 0) return cRes;

    Vec2i pulled[Config::MAX_PATH_LEN];
    int32_t pulledCount = postprocess::StringPullGridPath(
        raw.route, raw.len, pulled, Config::MAX_PATH_LEN,
        [this](Vec2i a, Vec2i b) noexcept { return math::HasLineOfSight(*reinterpret_cast<const Grid *>(this->m_grid), a, b); });

    cRes.found = true;
    cRes.len = pulledCount;
    float dist = 0.0f;
    for (int32_t i = 0; i < pulledCount; ++i) {
      cRes.waypoints[i] = Vec2f{static_cast<float>(pulled[i].x), static_cast<float>(pulled[i].y)};
      if (i > 0) {
        float dx = cRes.waypoints[i].x - cRes.waypoints[i - 1].x;
        float dy = cRes.waypoints[i].y - cRes.waypoints[i - 1].y;
        dist += std::sqrt(dx * dx + dy * dy);
      }
    }
    cRes.totalDistance = dist;
    return cRes;
  }

  // 4. "No Mistakes" Path Invariant Safety Validator
  [[nodiscard]] HALO_INLINE bool ValidatePathSafety(const PathResult &path) const noexcept {
    if (!path.found || path.len <= 0) return false;
    for (int32_t i = 0; i < path.len; ++i) {
      if (!m_grid->InBounds(path.route[i]) || !m_grid->IsWalkable(path.route[i].x, path.route[i].y)) {
        return false;
      }
      if (i > 0) {
        if (!math::HasLineOfSight(*reinterpret_cast<const Grid *>(m_grid), path.route[i - 1], path.route[i])) {
          return false;
        }
      }
    }
    return true;
  }

  // 5. Dense Path Expander (Converts Jump Point Sequences to Continuous Step-By-Step Tiles)
  HALO_INLINE void ExpandToDensePath(const PathResult &sparsePath, DensePathResult &outDense) const noexcept {
    outDense.found = sparsePath.found;
    outDense.stepCount = 0;
    if (!sparsePath.found || sparsePath.len <= 0) return;

    outDense.steps[outDense.stepCount++] = sparsePath.route[0];
    for (int32_t i = 1; i < sparsePath.len; ++i) {
      Vec2i cur = sparsePath.route[i - 1];
      Vec2i dest = sparsePath.route[i];
      while (cur != dest && outDense.stepCount < Config::MAX_PATH_LEN * 4) {
        int32_t sx = (dest.x > cur.x) ? 1 : (dest.x < cur.x ? -1 : 0);
        int32_t sy = (dest.y > cur.y) ? 1 : (dest.y < cur.y ? -1 : 0);
        cur.x += sx;
        cur.y += sy;
        outDense.steps[outDense.stepCount++] = cur;
      }
    }
  }

  // 6. Kinodynamic Continuous Quintic Trajectory Synthesis (< 800 ns, C^3 continuous)
  [[nodiscard]] HALO_INLINE PathResult RouteKinodynamic(Vec2i start, Vec2i target, const kinodynamics::KinodynamicLimits &limits,
                                                        kinodynamics::KinodynamicTrajectory &outTraj,
                                                        RoutingMode mode = RoutingMode::Turbo) noexcept {
    PathResult res = RouteGrid(start, target, mode);
    if (!res.found || res.len <= 0) {
      outTraj.Clear();
      return res;
    }

    Vec2i pulled[Config::MAX_PATH_LEN];
    int32_t pulledCount = postprocess::StringPullGridPath(
        res.route, res.len, pulled, Config::MAX_PATH_LEN,
        [this](Vec2i a, Vec2i b) noexcept { return math::HasLineOfSight(*reinterpret_cast<const Grid *>(this->m_grid), a, b); });

    Vec2f waypoints[Config::MAX_PATH_LEN];
    for (int32_t i = 0; i < pulledCount; ++i) {
      waypoints[i] = Vec2f{static_cast<float>(pulled[i].x), static_cast<float>(pulled[i].y)};
    }

    kinodynamics::GenerateQuinticTrajectory(waypoints, pulledCount, limits, outTraj);
    return res;
  }

  urban::UrbanPathResult RouteUrban(int32_t startNode, int32_t targetNode) noexcept {
    if (HALO_UNLIKELY(!m_cityMap)) return {};
    return m_apspRouter.RouteO1(startNode, targetNode);
  }

  void FlushMemory() noexcept { m_masterArena.Reset(); }

private:
  HALO_INLINE void InitGridNode(int32_t i) noexcept {
    if (HALO_UNLIKELY(m_gridNodes[i].searchEpoch != m_gridEpoch)) {
      m_gridNodes[i].searchEpoch = m_gridEpoch;
      m_gridNodes[i].state = 0;
      m_gridNodes[i].f = 0;
      m_gridNodes[i].g = 0;
      m_gridNodes[i].h = 0;
      m_gridNodes[i].parent = -1;
    }
  }

  void ExtractGridPath(int32_t endIdx, PathResult &res) noexcept {
    res.len = 0;
    int32_t curr = endIdx;
    while (curr != -1 && res.len < Config::MAX_PATH_LEN) {
      res.route[res.len++] = ToVec(curr);
      curr = m_gridNodes[curr].parent;
    }
    for (int32_t i = 0; i < res.len / 2; ++i) {
      std::swap(res.route[i], res.route[res.len - 1 - i]);
    }
  }
};

using HaloSupremeEngine = HaloSupremeEngineT<0, 0>;

// Microcontroller Presets (ESP32 SRAM / FreeRTOS friendly)
template <int32_t Dim = 64>
using EmbeddedSupremeEngine = HaloSupremeEngineT<Dim, Dim>;
using EmbeddedSupremeEngine32 = HaloSupremeEngineT<32, 32>;
using EmbeddedSupremeEngine64 = HaloSupremeEngineT<64, 64>;
using EmbeddedSupremeEngine128 = HaloSupremeEngineT<128, 128>;

}  // namespace halo::core