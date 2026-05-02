#pragma once

#include "../core/halo_memory.h"
#include "../utils/halo_types.h"
#include "halo_graph.h"

namespace halo::urban {

class QuantumApspRouter {
  int32_t m_nodes;
  int32_t *m_next;
  int32_t *m_dist;

public:
  void Precompute(CityMap &map, halo::memory::ArenaAllocator &arena) noexcept {
    m_nodes = map.GetNodeCount();

    m_next = arena.AllocateArray<int32_t>(m_nodes * m_nodes);
    m_dist = arena.AllocateArray<int32_t>(m_nodes * m_nodes);

    for (int i = 0; i < m_nodes * m_nodes; ++i) {
      m_dist[i] = 1000000000;
      m_next[i] = -1;
    }

    for (int i = 0; i < m_nodes; ++i) {
      m_dist[i * m_nodes + i] = 0;
      m_next[i * m_nodes + i] = i;
    }

    RoadEdge *edges = map.GetEdges();
    CityIntersection *nodes = map.GetNodes();

    for (int i = 0; i < m_nodes; ++i) {
      for (int32_t e = nodes[i].headEdge; e != -1; e = edges[e].nextEdge) {
        int32_t v = edges[e].targetNode;
        m_dist[i * m_nodes + v] = edges[e].moveCostFP;
        m_next[i * m_nodes + v] = v;
      }
    }

    for (int k = 0; k < m_nodes; ++k) {
      for (int i = 0; i < m_nodes; ++i) {

        if (m_dist[i * m_nodes + k] == 1000000000)
          continue;

        for (int j = 0; j < m_nodes; ++j) {
          int32_t newDist = m_dist[i * m_nodes + k] + m_dist[k * m_nodes + j];
          if (m_dist[i * m_nodes + j] > newDist) {
            m_dist[i * m_nodes + j] = newDist;
            m_next[i * m_nodes + j] = m_next[i * m_nodes + k];
          }
        }
      }
    }
  }

  UrbanPathResult RouteO1(int32_t start, int32_t target) noexcept {
    UrbanPathResult res;

    if (start < 0 || start >= m_nodes || target < 0 || target >= m_nodes)
      return res;
    if (m_next[start * m_nodes + target] == -1)
      return res;

    res.found = true;
    int32_t curr = start;

    while (curr != target && res.len < 1024) {
      res.route[res.len++] = curr;
      curr = m_next[curr * m_nodes + target];
    }
    res.route[res.len++] = target;
    return res;
  }
};

} // namespace halo::urban