#pragma once

#include "../navigation/halo_apsp.h"
#include "../navigation/halo_jps_plus.h"
#include "../utils/halo_heap.h"
#include "../utils/halo_math.h"
#include "../utils/halo_types.h"
#include "halo_memory.h"
#include <algorithm>
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

public:
  HaloSupremeEngineT() noexcept = default;

  [[nodiscard]] size_t GetMasterArenaCapacity() const noexcept { return m_masterArena.GetCapacity(); }
  [[nodiscard]] size_t GetMasterArenaOffset() const noexcept { return m_masterArena.GetOffset(); }

  [[gnu::noinline]] [[gnu::cold]] void BootSystem(GridT<W, H> *grid, size_t megaBytesRAM) {
    m_masterArena.Init(megaBytesRAM * 1024 * 1024);
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

  [[gnu::noinline]] [[gnu::cold]] void BootSystem(GridT<W, H> *grid, urban::CityMap *cityMap, size_t megaBytesRAM) {
    BootSystem(grid, megaBytesRAM);
    m_cityMap = cityMap;
    if (m_cityMap) {
      m_apspRouter.Precompute(*m_cityMap, m_masterArena);
    }
  }

  HALO_INLINE PathResult RouteGrid(Vec2i start, Vec2i target) noexcept {
    PathResult res;
    if (HALO_UNLIKELY(!m_grid || !m_grid->InBounds(start) ||
                      !m_grid->InBounds(target) ||
                      !m_grid->IsWalkable(ToIndex(start.x, start.y)) ||
                      !m_grid->IsWalkable(ToIndex(target.x, target.y)))) {
      return res;
    }

    if (HALO_UNLIKELY(start == target)) {
      res.found = true;
      res.len = 1;
      res.route[0] = start;
      return res;
    }

    ++m_gridEpoch;
    m_gridHeap.Clear();
    int32_t startDist = math::OctileDistanceFP_TieBreak(start, target, start);
    int32_t sIdx = ToIndex(start.x, start.y);

    InitGridNode(sIdx);
    m_gridNodes[sIdx] = {0, startDist, startDist, 0, -1, sIdx, m_gridEpoch, 1, {0, 0, 0}};
    m_gridHeap.Push(sIdx);

    while (HALO_LIKELY(!m_gridHeap.Empty())) {
      int32_t cIdx = m_gridHeap.Pop();
      PathNode &cNode = m_gridNodes[cIdx];
      cNode.state = 2; // Closed
      res.expanded++;

      Vec2i cPos = ToVec(cIdx);
      if (HALO_UNLIKELY(cPos == target)) {
        res.found = true;
        res.cost = cNode.g;
        ExtractGridPath(cIdx, res);
        return res;
      }

      int32_t diffX = target.x - cPos.x;
      int32_t diffY = target.y - cPos.y;

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
          int32_t projDist = (dir.y == 0 && dir.x * diffX > 0) ? std::abs(diffX) :
                             (dir.x == 0 && dir.y * diffY > 0) ? std::abs(diffY) : 0;
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

          int32_t nIdx = ToIndex(nPos.x, nPos.y);
          HALO_PREFETCH(&m_gridNodes[nIdx]);

          InitGridNode(nIdx);
          if (HALO_UNLIKELY(m_gridNodes[nIdx].state == 2)) continue;

          PathNode &nNode = m_gridNodes[nIdx];
          int32_t g = cNode.g + (step * Direction::CostFP[i]);

          if (HALO_LIKELY(nNode.state == 0 || g < nNode.g)) {
            nNode.g = g;
            nNode.h = math::OctileDistanceFP_TieBreak(nPos, target, start);
            nNode.parent = cIdx;
            nNode.index = nIdx;

            int32_t w = math::GetDynamicWeightFP(startDist, nNode.h);
            nNode.f = g + static_cast<int32_t>((static_cast<int64_t>(w) * nNode.h) >> 10) + m_grid->GetPenalty(nIdx);

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

  urban::UrbanPathResult RouteUrban(int32_t startNode, int32_t targetNode) noexcept {
    if (HALO_UNLIKELY(!m_cityMap)) return {};
    return m_apspRouter.RouteO1(startNode, targetNode);
  }

  void FlushMemory() noexcept {
    m_masterArena.Reset();
  }

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

} // namespace halo::core