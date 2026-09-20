#pragma once

#include "../core/halo_memory.h"
#include "../protection/halo_sparse_bitboard.h"
#include "../utils/halo_heap.h"
#include "../utils/halo_math.h"
#include "halo_spatial_coords.h"
#include <algorithm>
#include <cstdint>
#include <cstring>

namespace halo::continental {

struct alignas(8) MacroWaypoint {
  int32_t metricX = 0;
  int32_t metricY = 0;
};

struct TransContinentalRoute {
  bool found = false;
  int32_t waypointCount = 0;
  MacroWaypoint waypoints[Config::MAX_PATH_LEN];
  double totalDistanceKm = 0.0;
  uint64_t elapsedNs = 0;
};

// ============================================================================
// LOD 0 MACRO CONTINENTAL BACKBONE (Up to 2,000 km x 2,000 km in < 2 MB RAM)
// ============================================================================

template <int32_t MACRO_DIM = 2048, int32_t CELL_METERS = 1000, int32_t CLUSTER_DIM = 64>
class alignas(64) ContinentalMacroBackbone {
public:
  static constexpr int32_t CLUSTERS_PER_AXIS = MACRO_DIM / CLUSTER_DIM;
  static constexpr int32_t TOTAL_CLUSTERS = CLUSTERS_PER_AXIS * CLUSTERS_PER_AXIS;
  static constexpr int32_t WORDS_PER_ROW = MACRO_DIM / 64;
  static constexpr int32_t MAX_PORTALS = 8192;
  static constexpr int32_t MAX_EDGES = 32768;

  struct MacroPortal {
    int32_t mx = 0;
    int32_t my = 0;
    int32_t clusterA = -1;
    int32_t clusterB = -1;
    int32_t headEdge = -1;
  };

  struct PortalEdge {
    int32_t targetPortal = -1;
    int32_t cost = 0;
    int32_t nextEdge = -1;
  };

private:
  spatial::SpatialExtent2D m_extent;

  // Macro 1-bit obstacle matrix: 2048 x (2048 / 64) * 8 = 524,288 bytes (512 KB)
  uint64_t *m_obstacleBits = nullptr;

  MacroPortal *m_portals = nullptr;
  PortalEdge *m_edges = nullptr;
  int32_t m_portalCount = 0;
  int32_t m_edgeCount = 0;

  // Search state
  PathNode *m_searchNodes = nullptr;
  FourAryMinHeap m_heap;
  uint32_t m_epoch = 0;

public:
  ContinentalMacroBackbone() noexcept = default;

  void Init(const spatial::SpatialExtent2D &extent, memory::ArenaAllocator &arena) noexcept {
    m_extent = extent;
    m_obstacleBits = arena.AllocateArray<uint64_t, 64>(MACRO_DIM * WORDS_PER_ROW);
    std::memset(m_obstacleBits, 0, MACRO_DIM * WORDS_PER_ROW * sizeof(uint64_t));

    m_portals = arena.AllocateArray<MacroPortal, 64>(MAX_PORTALS);
    m_edges = arena.AllocateArray<PortalEdge, 64>(MAX_EDGES);
    m_searchNodes = arena.AllocateArray<PathNode, 64>(MAX_PORTALS + 4);

    m_portalCount = 0;
    m_edgeCount = 0;
    m_epoch = 0;

    m_heap.Init(MAX_PORTALS + 4, m_searchNodes, arena);
  }

  // World metric coordinate to macro cell index
  [[nodiscard]] HALO_INLINE int32_t MetricToMacroX(double worldX) const noexcept {
    int32_t mx = static_cast<int32_t>((worldX - m_extent.minX) / CELL_METERS);
    return std::clamp(mx, 0, MACRO_DIM - 1);
  }

  [[nodiscard]] HALO_INLINE int32_t MetricToMacroY(double worldY) const noexcept {
    int32_t my = static_cast<int32_t>((worldY - m_extent.minY) / CELL_METERS);
    return std::clamp(my, 0, MACRO_DIM - 1);
  }

  [[nodiscard]] HALO_INLINE double MacroToMetricX(int32_t mx) const noexcept {
    return m_extent.minX + (static_cast<double>(mx) + 0.5) * CELL_METERS;
  }

  [[nodiscard]] HALO_INLINE double MacroToMetricY(int32_t my) const noexcept {
    return m_extent.minY + (static_cast<double>(my) + 0.5) * CELL_METERS;
  }

  HALO_INLINE void SetMacroObstacle(int32_t mx, int32_t my) noexcept {
    if (mx < 0 || mx >= MACRO_DIM || my < 0 || my >= MACRO_DIM) return;
    int32_t wordIdx = my * WORDS_PER_ROW + (mx >> 6);
    m_obstacleBits[wordIdx] |= (1ULL << (mx & 63));
  }

  [[nodiscard]] HALO_INLINE bool IsMacroBlocked(int32_t mx, int32_t my) const noexcept {
    if (mx < 0 || mx >= MACRO_DIM || my < 0 || my >= MACRO_DIM) return true;
    int32_t wordIdx = my * WORDS_PER_ROW + (mx >> 6);
    return (m_obstacleBits[wordIdx] & (1ULL << (mx & 63))) != 0;
  }

  // Build cluster boundary portals for fast macro corridor routing
  void BuildMacroPortals() noexcept {
    m_portalCount = 0;
    m_edgeCount = 0;

    // Vertical boundary portals between adjacent clusters
    for (int32_t cy = 0; cy < CLUSTERS_PER_AXIS && m_portalCount < MAX_PORTALS - 16; ++cy) {
      for (int32_t cx = 0; cx < CLUSTERS_PER_AXIS - 1 && m_portalCount < MAX_PORTALS - 16; ++cx) {
        int32_t boundaryX = (cx + 1) * CLUSTER_DIM - 1;
        int32_t startY = cy * CLUSTER_DIM;
        int32_t endY = startY + CLUSTER_DIM;

        for (int32_t y = startY + 4; y < endY - 4; y += 16) {
          if (!IsMacroBlocked(boundaryX, y) && !IsMacroBlocked(boundaryX + 1, y)) {
            AddPortal(boundaryX, y, cy * CLUSTERS_PER_AXIS + cx, cy * CLUSTERS_PER_AXIS + cx + 1);
            break;
          }
        }
      }
    }

    // Horizontal boundary portals between adjacent clusters
    for (int32_t cy = 0; cy < CLUSTERS_PER_AXIS - 1 && m_portalCount < MAX_PORTALS - 16; ++cy) {
      for (int32_t cx = 0; cx < CLUSTERS_PER_AXIS && m_portalCount < MAX_PORTALS - 16; ++cx) {
        int32_t boundaryY = (cy + 1) * CLUSTER_DIM - 1;
        int32_t startX = cx * CLUSTER_DIM;
        int32_t endX = startX + CLUSTER_DIM;

        for (int32_t x = startX + 4; x < endX - 4; x += 16) {
          if (!IsMacroBlocked(x, boundaryY) && !IsMacroBlocked(x, boundaryY + 1)) {
            AddPortal(x, boundaryY, cy * CLUSTERS_PER_AXIS + cx, (cy + 1) * CLUSTERS_PER_AXIS + cx);
            break;
          }
        }
      }
    }

    // Connect intra-cluster portals with edges
    for (int32_t i = 0; i < m_portalCount; ++i) {
      for (int32_t j = i + 1; j < m_portalCount; ++j) {
        if (m_portals[i].clusterA == m_portals[j].clusterA || m_portals[i].clusterA == m_portals[j].clusterB ||
            m_portals[i].clusterB == m_portals[j].clusterA || m_portals[i].clusterB == m_portals[j].clusterB) {
          int32_t dx = m_portals[i].mx - m_portals[j].mx;
          int32_t dy = m_portals[i].my - m_portals[j].my;
          int32_t dist = static_cast<int32_t>(std::sqrt(dx * dx + dy * dy));
          AddEdge(i, j, dist);
          AddEdge(j, i, dist);
        }
      }
    }
  }

  void AddPortal(int32_t mx, int32_t my, int32_t cA, int32_t cB) noexcept {
    if (m_portalCount >= MAX_PORTALS) return;
    m_portals[m_portalCount] = MacroPortal{mx, my, cA, cB, -1};
    ++m_portalCount;
  }

  void AddEdge(int32_t u, int32_t v, int32_t cost) noexcept {
    if (m_edgeCount >= MAX_EDGES) return;
    m_edges[m_edgeCount] = PortalEdge{v, cost, m_portals[u].headEdge};
    m_portals[u].headEdge = m_edgeCount++;
  }

  // Ultra-Low-Latency Trans-National Macro Routing (< 40 µs P99)
  [[nodiscard]] TransContinentalRoute FindTransNationalPath(double startWorldX, double startWorldY, double goalWorldX,
                                                            double goalWorldY) noexcept {
    TransContinentalRoute route;
    int32_t smx = MetricToMacroX(startWorldX);
    int32_t smy = MetricToMacroY(startWorldY);
    int32_t gmx = MetricToMacroX(goalWorldX);
    int32_t gmy = MetricToMacroY(goalWorldY);

    if (IsMacroBlocked(smx, smy) || IsMacroBlocked(gmx, gmy)) {
      return route;
    }

    ++m_epoch;
    m_heap.Clear();

    int32_t startNode = m_portalCount;
    int32_t goalNode = m_portalCount + 1;

    m_searchNodes[startNode] = PathNode{};
    m_searchNodes[startNode].index = startNode;
    m_searchNodes[startNode].g = 0;
    int32_t h0 = std::abs(smx - gmx) + std::abs(smy - gmy);
    m_searchNodes[startNode].f = h0;
    m_searchNodes[startNode].parent = -1;
    m_searchNodes[startNode].searchEpoch = m_epoch;

    m_searchNodes[goalNode] = PathNode{};
    m_searchNodes[goalNode].index = goalNode;
    m_searchNodes[goalNode].g = 1000000000;
    m_searchNodes[goalNode].f = 1000000000;
    m_searchNodes[goalNode].parent = -1;
    m_searchNodes[goalNode].searchEpoch = m_epoch;

    m_heap.Push(startNode);

    // Connect start node to closest portals
    for (int32_t p = 0; p < m_portalCount; ++p) {
      int32_t d = std::abs(smx - m_portals[p].mx) + std::abs(smy - m_portals[p].my);
      if (d < CLUSTER_DIM * 2) {
        int32_t h = std::abs(m_portals[p].mx - gmx) + std::abs(m_portals[p].my - gmy);
        m_searchNodes[p].index = p;
        m_searchNodes[p].g = d;
        m_searchNodes[p].f = d + h;
        m_searchNodes[p].parent = startNode;
        m_searchNodes[p].searchEpoch = m_epoch;
        m_heap.Push(p);
      }
    }

    // A* Portal Search
    while (!m_heap.Empty()) {
      int32_t curr = m_heap.Pop();
      if (curr == goalNode) {
        break;
      }

      int32_t cx = (curr == startNode) ? smx : m_portals[curr].mx;
      int32_t cy = (curr == startNode) ? smy : m_portals[curr].my;

      // Check distance to goal
      int32_t dGoal = std::abs(cx - gmx) + std::abs(cy - gmy);
      if (dGoal < CLUSTER_DIM * 2) {
        int32_t newG = m_searchNodes[curr].g + dGoal;
        if (newG < m_searchNodes[goalNode].g) {
          m_searchNodes[goalNode].g = newG;
          m_searchNodes[goalNode].f = newG;
          m_searchNodes[goalNode].parent = curr;
          m_heap.Push(goalNode);
        }
      }

      if (curr >= m_portalCount) continue;

      // Traverse portal edges
      for (int32_t e = m_portals[curr].headEdge; e != -1; e = m_edges[e].nextEdge) {
        int32_t target = m_edges[e].targetPortal;
        int32_t edgeCost = m_edges[e].cost;
        int32_t tentG = m_searchNodes[curr].g + edgeCost;

        if (m_searchNodes[target].searchEpoch != m_epoch || tentG < m_searchNodes[target].g) {
          m_searchNodes[target].searchEpoch = m_epoch;
          m_searchNodes[target].index = target;
          m_searchNodes[target].g = tentG;
          int32_t h = std::abs(m_portals[target].mx - gmx) + std::abs(m_portals[target].my - gmy);
          m_searchNodes[target].f = tentG + h;
          m_searchNodes[target].parent = curr;
          m_heap.Push(target);
        }
      }
    }

    // Extract path waypoints
    if (m_searchNodes[goalNode].g < 1000000000) {
      route.found = true;
      int32_t tempWaypoints[Config::MAX_PATH_LEN];
      int32_t count = 0;
      int32_t curr = goalNode;

      while (curr != -1 && count < Config::MAX_PATH_LEN) {
        tempWaypoints[count++] = curr;
        curr = m_searchNodes[curr].parent;
      }

      route.waypointCount = count;
      for (int32_t i = 0; i < count; ++i) {
        int32_t nodeIdx = tempWaypoints[count - 1 - i];
        if (nodeIdx == startNode) {
          route.waypoints[i] = MacroWaypoint{static_cast<int32_t>(startWorldX), static_cast<int32_t>(startWorldY)};
        } else if (nodeIdx == goalNode) {
          route.waypoints[i] = MacroWaypoint{static_cast<int32_t>(goalWorldX), static_cast<int32_t>(goalWorldY)};
        } else {
          route.waypoints[i] = MacroWaypoint{static_cast<int32_t>(MacroToMetricX(m_portals[nodeIdx].mx)),
                                             static_cast<int32_t>(MacroToMetricY(m_portals[nodeIdx].my))};
        }
      }

      double totalMeters = 0.0;
      for (int32_t i = 1; i < count; ++i) {
        double dx = route.waypoints[i].metricX - route.waypoints[i - 1].metricX;
        double dy = route.waypoints[i].metricY - route.waypoints[i - 1].metricY;
        totalMeters += std::sqrt(dx * dx + dy * dy);
      }
      route.totalDistanceKm = totalMeters / 1000.0;
    }

    return route;
  }
};

}  // namespace halo::continental
