#pragma once

#include "../core/halo_memory.h"
#include "../utils/halo_math.h"
#include "../utils/halo_types.h"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstring>

namespace halo::urban {

struct RoadEdge {
  int32_t targetNode = -1;
  int32_t moveCostFP = 0;
  int32_t nextEdge = -1;
};

struct alignas(64) CityIntersection {
  int32_t x_fp = 0;
  int32_t y_fp = 0;
  int32_t g = 0;
  int32_t h = 0;
  int32_t f = 0;
  int32_t parent = -1;
  int32_t headEdge = -1;
  uint32_t searchEpoch = 0;
  uint8_t state = 0;
  uint8_t _pad[31] = {};
};
static_assert(sizeof(CityIntersection) == 64, "CityIntersection must be exactly 64 bytes");

class alignas(64) CityMap {
private:
  CityIntersection *m_nodes = nullptr;
  RoadEdge *m_edges = nullptr;
  int32_t m_nCount = 0;
  int32_t m_eCount = 0;
  int32_t m_maxNodes = 0;
  int32_t m_maxEdges = 0;

public:
  CityMap() noexcept = default;

  void Init(int32_t maxN, int32_t maxE, memory::ArenaAllocator &arena) noexcept {
    m_maxNodes = maxN;
    m_maxEdges = maxE;
    m_nodes = arena.AllocateArray<CityIntersection, 64>(maxN);
    m_edges = arena.AllocateArray<RoadEdge, 64>(maxE);
    m_nCount = 0;
    m_eCount = 0;
  }

  int32_t AddIntersection(float x, float y) noexcept {
    assert(m_nCount < m_maxNodes && "CityMap: Max intersections exceeded");
    int32_t id = m_nCount++;

    m_nodes[id].x_fp = static_cast<int32_t>(x * Config::FP_MULT);
    m_nodes[id].y_fp = static_cast<int32_t>(y * Config::FP_MULT);
    m_nodes[id].g = 0;
    m_nodes[id].h = 0;
    m_nodes[id].f = 0;
    m_nodes[id].parent = -1;
    m_nodes[id].headEdge = -1;
    m_nodes[id].searchEpoch = 0;
    m_nodes[id].state = 0;
    return id;
  }

  void AddRoad(int32_t f, int32_t t, float traffic) noexcept {
    assert(m_eCount < m_maxEdges && "CityMap: Max edges exceeded");
    int32_t dx = std::abs(m_nodes[t].x_fp - m_nodes[f].x_fp);
    int32_t dy = std::abs(m_nodes[t].y_fp - m_nodes[f].y_fp);
    int32_t mn = (dx < dy) ? dx : dy;
    int32_t mx = (dx > dy) ? dx : dy;

    int32_t distFP = mx + ((Config::SQRT2_MINUS_1_FP * mn) >> 10);
    int32_t edgeId = m_eCount++;
    m_edges[edgeId] = {t, static_cast<int32_t>(static_cast<float>(distFP) * traffic), m_nodes[f].headEdge};
    m_nodes[f].headEdge = edgeId;
  }

  void AddTwoWay(int32_t a, int32_t b, float traffic) noexcept {
    AddRoad(a, b, traffic);
    AddRoad(b, a, traffic);
  }

  [[nodiscard]] CityIntersection *GetNodes() const noexcept { return m_nodes; }
  [[nodiscard]] RoadEdge *GetEdges() const noexcept { return m_edges; }
  [[nodiscard]] int32_t GetNodeCount() const noexcept { return m_nCount; }
  [[nodiscard]] int32_t GetEdgeCount() const noexcept { return m_eCount; }
};

struct UrbanPathResult {
  bool found = false;
  int32_t len = 0;
  int32_t route[Config::MAX_PATH_LEN] = {};
};

// Cache-Conscious 4-ary Min Heap for Urban Routing
class alignas(64) UrbanMinHeap {
private:
  CityIntersection *m_pool = nullptr;
  int32_t m_cap = 0;
  int32_t m_size = 0;
  int32_t *m_heap = nullptr;
  int32_t *m_pos = nullptr;

  [[nodiscard]] HALO_INLINE bool Less(int32_t a, int32_t b) const noexcept {
    return (m_pool[a].f == m_pool[b].f) ? (m_pool[a].g > m_pool[b].g) : (m_pool[a].f < m_pool[b].f);
  }

public:
  UrbanMinHeap() noexcept = default;

  void Init(int32_t cap, CityIntersection *pool, memory::ArenaAllocator &arena) noexcept {
    m_pool = pool;
    m_cap = cap;
    m_size = 0;
    m_heap = arena.AllocateArray<int32_t, 64>(cap);
    m_pos = arena.AllocateArray<int32_t, 64>(cap);
    if (m_pos) {
      std::memset(m_pos, 0xFF, static_cast<size_t>(cap) * sizeof(int32_t));
    }
  }

  inline void Clear() noexcept { m_size = 0; }
  [[nodiscard]] inline bool Empty() const noexcept { return m_size == 0; }

  inline void Push(int32_t idx) noexcept {
    assert(m_size < m_cap);
    SiftUp(m_size++, idx);
  }

  [[nodiscard]] inline int32_t Pop() noexcept {
    assert(m_size > 0);
    int32_t top = m_heap[0];
    m_pos[top] = -1;
    if (--m_size > 0) {
      SiftDown(0, m_heap[m_size]);
    }
    return top;
  }

  inline void DecreaseKey(int32_t idx) noexcept {
    int32_t p = m_pos[idx];
    if (HALO_LIKELY(p >= 0)) {
      SiftUp(p, idx);
    }
  }

private:
  HALO_INLINE void SiftUp(int32_t pos, int32_t idx) noexcept {
    while (pos > 0) {
      int32_t p = (pos - 1) >> 2;
      int32_t pIdx = m_heap[p];
      if (Less(idx, pIdx)) {
        m_heap[pos] = pIdx;
        m_pos[pIdx] = pos;
        pos = p;
      } else {
        break;
      }
    }
    m_heap[pos] = idx;
    m_pos[idx] = pos;
  }

  HALO_INLINE void SiftDown(int32_t pos, int32_t idx) noexcept {
    while (true) {
      int32_t firstChild = (pos << 2) + 1;
      if (firstChild >= m_size) break;

      int32_t bestChild = firstChild;
      int32_t bestIdx = m_heap[firstChild];

      int32_t c2 = firstChild + 1;
      if (c2 < m_size && Less(m_heap[c2], bestIdx)) {
        bestChild = c2;
        bestIdx = m_heap[c2];
      }
      int32_t c3 = firstChild + 2;
      if (c3 < m_size && Less(m_heap[c3], bestIdx)) {
        bestChild = c3;
        bestIdx = m_heap[c3];
      }
      int32_t c4 = firstChild + 3;
      if (c4 < m_size && Less(m_heap[c4], bestIdx)) {
        bestChild = c4;
        bestIdx = m_heap[c4];
      }

      if (Less(bestIdx, idx)) {
        m_heap[pos] = bestIdx;
        m_pos[bestIdx] = pos;
        pos = bestChild;
      } else {
        break;
      }
    }
    m_heap[pos] = idx;
    m_pos[idx] = pos;
  }
};

class alignas(64) UrbanPathfinder {
private:
  CityMap *m_map = nullptr;
  UrbanMinHeap m_open;
  uint32_t m_epoch = 0;

public:
  UrbanPathfinder() noexcept = default;

  void Init(CityMap *map, memory::ArenaAllocator &arena) noexcept {
    m_map = map;
    m_open.Init(map->GetNodeCount(), map->GetNodes(), arena);
  }

  UrbanPathResult RouteTraffic(int32_t startId, int32_t targetId) noexcept {
    UrbanPathResult res;
    if (HALO_UNLIKELY(!m_map || startId < 0 || startId >= m_map->GetNodeCount() || targetId < 0 || targetId >= m_map->GetNodeCount())) {
      return res;
    }

    CityIntersection *nodes = m_map->GetNodes();
    RoadEdge *edges = m_map->GetEdges();
    ++m_epoch;
    m_open.Clear();

    InitNode(nodes[startId]);
    nodes[startId].g = 0;

    int32_t dx = std::abs(nodes[startId].x_fp - nodes[targetId].x_fp);
    int32_t dy = std::abs(nodes[startId].y_fp - nodes[targetId].y_fp);
    int32_t mn = (dx < dy) ? dx : dy;
    int32_t mx = (dx > dy) ? dx : dy;
    nodes[startId].h = mx + ((Config::SQRT2_MINUS_1_FP * mn) >> 10);
    nodes[startId].f = nodes[startId].h;
    nodes[startId].parent = -1;
    nodes[startId].state = 1;
    m_open.Push(startId);

    while (!m_open.Empty()) {
      int32_t cId = m_open.Pop();
      CityIntersection &curr = nodes[cId];
      curr.state = 2;

      if (cId == targetId) {
        res.found = true;
        int32_t t = targetId;
        while (t != -1 && res.len < Config::MAX_PATH_LEN) {
          res.route[res.len++] = t;
          t = nodes[t].parent;
        }
        for (int32_t i = 0; i < res.len / 2; ++i) {
          std::swap(res.route[i], res.route[res.len - 1 - i]);
        }
        return res;
      }

      for (int32_t e = curr.headEdge; e != -1; e = edges[e].nextEdge) {
        RoadEdge &road = edges[e];
        int32_t nId = road.targetNode;
        CityIntersection &next = nodes[nId];
        InitNode(next);
        if (next.state == 2) continue;

        int32_t g = curr.g + road.moveCostFP;
        if (next.state == 0 || g < next.g) {
          next.g = g;
          int32_t ndx = std::abs(next.x_fp - nodes[targetId].x_fp);
          int32_t ndy = std::abs(next.y_fp - nodes[targetId].y_fp);
          int32_t nmn = (ndx < ndy) ? ndx : ndy;
          int32_t nmx = (ndx > ndy) ? ndx : ndy;
          next.h = nmx + ((Config::SQRT2_MINUS_1_FP * nmn) >> 10);
          next.parent = cId;
          next.f = next.g + next.h;

          if (next.state == 0) {
            next.state = 1;
            m_open.Push(nId);
          } else {
            m_open.DecreaseKey(nId);
          }
        }
      }
    }
    return res;
  }

private:
  HALO_INLINE void InitNode(CityIntersection &n) noexcept {
    if (n.searchEpoch != m_epoch) {
      n.searchEpoch = m_epoch;
      n.state = 0;
      n.g = 0;
      n.h = 0;
      n.f = 0;
      n.parent = -1;
    }
  }
};

}  // namespace halo::urban