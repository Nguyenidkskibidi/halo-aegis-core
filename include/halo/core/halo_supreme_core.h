#pragma once
#include "halo_apsp.h"
#include "halo_heap.h"
#include "halo_jps_plus.h"
#include "halo_math.h"
#include "halo_memory.h"
#include "halo_types.h"

namespace halo::core {

class HaloSupremeEngine {
private:
  memory::ArenaAllocator m_masterArena;

  Grid *m_grid = nullptr;
  urban::CityMap *m_cityMap = nullptr;

  PathNode *m_gridNodes = nullptr;
  BinaryMinHeap m_gridHeap;
  JpsPlusEngine m_jpsEngine;
  urban::QuantumApspRouter m_apspRouter;

  uint32_t m_gridEpoch = 0;

public:
  void BootSystem(Grid *grid, urban::CityMap *cityMap, size_t megaBytesRAM) {

    m_masterArena.Init(megaBytesRAM * 1024 * 1024);

    m_grid = grid;
    m_cityMap = cityMap;

    if (m_grid) {
      int32_t gridTot = m_grid->Size();
      m_gridNodes = m_masterArena.AllocateArray<PathNode>(gridTot);
      std::memset(m_gridNodes, 0, gridTot * sizeof(PathNode));
      m_gridHeap.Init(gridTot, m_gridNodes, m_masterArena);

      m_jpsEngine.Precompute(m_grid, m_masterArena);
    }

    if (m_cityMap) {

      m_apspRouter.Precompute(*m_cityMap, m_masterArena);
    }
  }

  PathResult RouteGrid(Vec2i start, Vec2i target) noexcept {
    PathResult res;
    if (UNLIKELY(!m_grid || !m_grid->InBounds(start) ||
                 !m_grid->InBounds(target) ||
                 !m_grid->IsWalkable(m_grid->ToIndex(start)) ||
                 !m_grid->IsWalkable(m_grid->ToIndex(target))))
      return res;

    if (UNLIKELY(start == target)) {
      res.found = true;
      return res;
    }

    ++m_gridEpoch;
    m_gridHeap.Clear();
    int32_t startDist = math::OctileDistanceFP_TieBreak(start, target, start);
    int32_t bestF = 2000000000;
    int32_t sIdx = m_grid->ToIndex(start);

    InitGridNode(sIdx);
    m_gridNodes[sIdx] = {0,    startDist,   startDist, 0, -1,
                         sIdx, m_gridEpoch, 1,         {}};
    m_gridHeap.Push(sIdx);

    while (LIKELY(!m_gridHeap.Empty())) {
      int32_t cIdx = m_gridHeap.Pop();
      PathNode &cNode = m_gridNodes[cIdx];
      cNode.state = 2;
      res.expanded++;

      if (UNLIKELY(cNode.f >= bestF))
        continue;

      Vec2i cPos = m_grid->ToVec(cIdx);
      if (UNLIKELY(cPos == target)) {
        res.found = true;
        res.cost = cNode.g;
        bestF = std::min(bestF, cNode.g);
        ExtractGridPath(cIdx, res);
        return res;
      }

      bool valid = false;

#pragma clang loop unroll(full)
      for (int i = 0; i < 8; ++i) {

        int32_t jumpSteps = m_jpsEngine.GetJumpDistance(cIdx, i);
        if (jumpSteps == 0)
          continue;

        Vec2i nPos = cPos + Vec2i(Direction::Offsets[i].x * jumpSteps,
                                  Direction::Offsets[i].y * jumpSteps);
        int32_t nIdx = m_grid->ToIndex(nPos);

        HALO_PREFETCH(&m_gridNodes[nIdx]);

        InitGridNode(nIdx);
        if (UNLIKELY(m_gridNodes[nIdx].state == 2))
          continue;

        valid = true;
        PathNode &nNode = m_gridNodes[nIdx];

        int32_t g = cNode.g + (jumpSteps * Direction::CostFP[i]);

        if (LIKELY(nNode.state == 0 || g < nNode.g)) {

          nNode.g = g;
          nNode.h = math::OctileDistanceFP_TieBreak(nPos, target, start);
          nNode.parent = cIdx;
          nNode.index = nIdx;

          int32_t w = math::GetDynamicWeightFP(startDist, nNode.h);
          nNode.f = g + ((w * nNode.h) >> 10) + m_grid->GetPenalty(nIdx);

          if (nNode.state == 0) {
            nNode.state = 1;
            m_gridHeap.Push(nIdx);
          } else {
            m_gridHeap.DecreaseKey(nIdx);
          }
        }
      }
      if (UNLIKELY(!valid && cNode.parent != -1))
        m_grid->AddPenalty(cIdx, Config::DEAD_END_PENALTY);
    }
    return res;
  }

  urban::UrbanPathResult RouteUrban(int32_t startNode,
                                    int32_t targetNode) noexcept {

    if (UNLIKELY(!m_cityMap))
      return {};
    return m_apspRouter.RouteO1(startNode, targetNode);
  }

  void FlushMemory() noexcept { m_masterArena.Reset(); }

private:
  inline void InitGridNode(int32_t i) noexcept {
    if (UNLIKELY(m_gridNodes[i].searchEpoch != m_gridEpoch)) {
      m_gridNodes[i].searchEpoch = m_gridEpoch;
      m_gridNodes[i].state = 0;
    }
  }

  inline void ExtractGridPath(int32_t idx, PathResult &res) noexcept {
    res.len = 0;
    while (LIKELY(idx != -1) && LIKELY(res.len < Config::MAX_PATH_LEN)) {
      res.route[res.len++] = m_grid->ToVec(idx);
      idx = m_gridNodes[idx].parent;
    }
    for (int i = 0; i < res.len / 2; ++i) {
      std::swap(res.route[i], res.route[res.len - 1 - i]);
    }
  }
};

} // namespace halo::core