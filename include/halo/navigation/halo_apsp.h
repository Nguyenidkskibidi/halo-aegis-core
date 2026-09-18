#pragma once

#include "../core/halo_memory.h"
#include "../utils/halo_types.h"
#include "halo_graph.h"
#include <algorithm>
#include <cstdint>

namespace halo::urban {

class alignas(64) QuantumApspRouter {
private:
  int32_t m_nodes = 0;
  int32_t *m_next = nullptr;
  int32_t *m_dist = nullptr;

  static constexpr int32_t INF_DIST = 1000000000;

public:
  QuantumApspRouter() noexcept = default;

  [[gnu::cold]] [[gnu::noinline]] void Precompute(CityMap &map, halo::memory::ArenaAllocator &arena) noexcept {
    m_nodes = map.GetNodeCount();
    if (m_nodes <= 0) return;

    const size_t totalMatrixSize = static_cast<size_t>(m_nodes) * m_nodes;
    m_next = arena.AllocateArray<int32_t, 64>(totalMatrixSize);
    m_dist = arena.AllocateArray<int32_t, 64>(totalMatrixSize);

    for (size_t i = 0; i < totalMatrixSize; ++i) {
      m_dist[i] = INF_DIST;
      m_next[i] = -1;
    }

    for (int32_t i = 0; i < m_nodes; ++i) {
      m_dist[i * m_nodes + i] = 0;
      m_next[i * m_nodes + i] = i;
    }

    const RoadEdge *edges = map.GetEdges();
    const CityIntersection *nodes = map.GetNodes();

    for (int32_t i = 0; i < m_nodes; ++i) {
      for (int32_t e = nodes[i].headEdge; e != -1; e = edges[e].nextEdge) {
        int32_t v = edges[e].targetNode;
        m_dist[i * m_nodes + v] = edges[e].moveCostFP;
        m_next[i * m_nodes + v] = v;
      }
    }

    // Cache-friendly unit-stride Floyd-Warshall
    for (int32_t k = 0; k < m_nodes; ++k) {
      const int32_t kRow = k * m_nodes;
      for (int32_t i = 0; i < m_nodes; ++i) {
        const int32_t iRow = i * m_nodes;
        const int32_t dik = m_dist[iRow + k];
        if (dik == INF_DIST) continue;

        const int32_t nextIK = m_next[iRow + k];

        for (int32_t j = 0; j < m_nodes; ++j) {
          const int32_t dkj = m_dist[kRow + j];
          if (dkj == INF_DIST) continue;

          const int32_t newDist = dik + dkj;
          if (newDist < m_dist[iRow + j]) {
            m_dist[iRow + j] = newDist;
            m_next[iRow + j] = nextIK;
          }
        }
      }
    }
  }

  [[nodiscard]] UrbanPathResult RouteO1(int32_t start, int32_t target) const noexcept {
    UrbanPathResult res;
    if (HALO_UNLIKELY(start < 0 || start >= m_nodes || target < 0 || target >= m_nodes || !m_next)) {
      return res;
    }

    if (m_next[start * m_nodes + target] == -1) {
      return res;
    }

    res.found = true;
    int32_t curr = start;

    while (curr != target && res.len < Config::MAX_PATH_LEN) {
      res.route[res.len++] = curr;
      curr = m_next[curr * m_nodes + target];
      if (curr == -1) break;
    }
    if (res.len < Config::MAX_PATH_LEN) {
      res.route[res.len++] = target;
    }
    return res;
  }
};

} // namespace halo::urban