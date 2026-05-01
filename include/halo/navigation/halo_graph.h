/**
 * ============================================================================
 *  H.A.L.O.* URBAN — ALL-IN-ONE GRAPH ROUTING (FIXED-POINT)
 * ============================================================================
 */
#pragma once

// Sửa đường dẫn để nó lùi ra ngoài folder navigation rồi tìm vào folder đúng
#include "../utils/halo_types.h"
#include "../core/halo_memory.h"
#include "../utils/halo_math.h"

namespace halo::urban {

struct RoadEdge { int32_t targetNode, moveCostFP, nextEdge; };

// Căn lề 64-byte để tối ưu hóa Cache Line của CPU
struct HALO_ALIGN(64) CityIntersection {
    int32_t x_fp, y_fp, g, h, f, parent, headEdge;
    uint32_t searchEpoch; uint8_t state, _pad[31];
};

class CityMap {
    CityIntersection* m_nodes; RoadEdge* m_edges; int32_t m_nCount=0, m_eCount=0;
public:
    void Init(int32_t mN, int32_t mE, memory::ArenaAllocator& arena) {
        m_nodes = arena.AllocateArray<CityIntersection>(mN);
        m_edges = arena.AllocateArray<RoadEdge>(mE); m_nCount = 0; m_eCount = 0;
    }
    int32_t AddIntersection(float x, float y) {
        int32_t id = m_nCount++;
        // Sử dụng Config::FP_MULT để chuyển float sang Fixed-point
        m_nodes[id] = {static_cast<int32_t>(x * Config::FP_MULT), static_cast<int32_t>(y * Config::FP_MULT), 0, 0, 0, -1, -1, 0, 0, {}};
        return id;
    }
    void AddRoad(int32_t f, int32_t t, float traffic) {
        int32_t dx = std::abs(m_nodes[t].x_fp - m_nodes[f].x_fp), dy = std::abs(m_nodes[t].y_fp - m_nodes[f].y_fp);
        int32_t mn = dx < dy ? dx : dy, mx = dx > dy ? dx : dy;
        // Tính khoảng cách bằng công thức xấp xỉ nhanh (Octile distance)
        int32_t distFP = mx + ((Config::SQRT2_MINUS_1_FP * mn) >> 10);
        int32_t edgeId = m_eCount++;
        m_edges[edgeId] = {t, static_cast<int32_t>(distFP * traffic), m_nodes[f].headEdge};
        m_nodes[f].headEdge = edgeId;
    }
    void AddTwoWay(int32_t a, int32_t b, float t) { AddRoad(a,b,t); AddRoad(b,a,t); }
    CityIntersection* GetNodes() const { return m_nodes; }
    RoadEdge* GetEdges() const { return m_edges; }
    int32_t GetNodeCount() const { return m_nCount; }
};

struct UrbanPathResult { bool found = false; int32_t len = 0; int32_t route[Config::MAX_PATH_LEN]; };

// Heap tối ưu cho việc tìm kiếm đường đi ngắn nhất
class UrbanMinHeap {
    CityIntersection* m_pool; int32_t m_cap, m_size, *m_heap, *m_pos;
public:
    void Init(int32_t cap, CityIntersection* pool, memory::ArenaAllocator& arena) {
        m_pool = pool; m_cap = cap; m_size = 0;
        m_heap = arena.AllocateArray<int32_t>(cap); m_pos = arena.AllocateArray<int32_t>(cap);
        std::memset(m_pos, 0xFF, cap * sizeof(int32_t));
    }
    inline void Clear() { m_size = 0; }
    inline bool Empty() const { return m_size == 0; }
    inline void Push(int32_t idx) { SiftUp(m_size++, idx); }
    inline int32_t Pop() { int32_t t = m_heap[0]; m_pos[t] = -1; if (--m_size > 0) SiftDown(0, m_heap[m_size]); return t; }
    inline void DecreaseKey(int32_t idx) { SiftUp(m_pos[idx], idx); }
private:
    inline void SiftUp(int32_t pos, int32_t idx) {
        while (pos > 0) {
            int32_t p = (pos - 1) >> 1, pIdx = m_heap[p];
            if ((m_pool[idx].f == m_pool[pIdx].f) ? (m_pool[idx].g > m_pool[pIdx].g) : (m_pool[idx].f < m_pool[pIdx].f)) {
                m_heap[pos] = pIdx; m_pos[pIdx] = pos; pos = p;
            } else break;
        }
        m_heap[pos] = idx; m_pos[idx] = pos;
    }
    inline void SiftDown(int32_t pos, int32_t idx) {
        int32_t half = m_size >> 1;
        while (pos < half) {
            int32_t c = (pos << 1) + 1, cIdx = m_heap[c];
            if (c + 1 < m_size && ((m_pool[m_heap[c+1]].f == m_pool[cIdx].f) ? (m_pool[m_heap[c+1]].g > m_pool[cIdx].g) : (m_pool[m_heap[c+1]].f < m_pool[cIdx].f))) cIdx = m_heap[++c];
            if ((m_pool[idx].f == m_pool[cIdx].f) ? (m_pool[idx].g > m_pool[cIdx].g) : (m_pool[idx].f < m_pool[cIdx].f)) break;
            m_heap[pos] = cIdx; m_pos[cIdx] = pos; pos = c;
        }
        m_heap[pos] = idx; m_pos[idx] = pos;
    }
};

class UrbanPathfinder {
    CityMap* m_map; UrbanMinHeap m_open; uint32_t m_epoch = 0;
public:
    void Init(CityMap* map, memory::ArenaAllocator& arena) { m_map = map; m_open.Init(map->GetNodeCount(), map->GetNodes(), arena); }
    UrbanPathResult RouteTraffic(int32_t startId, int32_t targetId) {
        UrbanPathResult res; CityIntersection* nodes = m_map->GetNodes(); RoadEdge* edges = m_map->GetEdges();
        ++m_epoch; m_open.Clear();
        InitNode(nodes[startId]); nodes[startId].g = 0;
        
        int32_t dx = std::abs(nodes[startId].x_fp - nodes[targetId].x_fp), dy = std::abs(nodes[startId].y_fp - nodes[targetId].y_fp);
        nodes[startId].h = (dx > dy ? dx : dy) + ((Config::SQRT2_MINUS_1_FP * (dx < dy ? dx : dy)) >> 10);
        nodes[startId].f = nodes[startId].h; nodes[startId].parent = -1; nodes[startId].state = 1;
        m_open.Push(startId);

        while (!m_open.Empty()) {
            int32_t cId = m_open.Pop(); CityIntersection& curr = nodes[cId]; curr.state = 2;
            if (cId == targetId) {
                res.found = true; int32_t t = targetId;
                while (t != -1 && res.len < Config::MAX_PATH_LEN) { res.route[res.len++] = t; t = nodes[t].parent; }
                for(int i=0; i<res.len/2; ++i) std::swap(res.route[i], res.route[res.len - 1 - i]);
                return res;
            }
            for (int32_t e = curr.headEdge; e != -1; e = edges[e].nextEdge) {
                RoadEdge& road = edges[e]; int32_t nId = road.targetNode; CityIntersection& next = nodes[nId];
                InitNode(next); if (next.state == 2) continue;
                int32_t g = curr.g + road.moveCostFP;
                if (next.state == 0 || g < next.g) {
                    next.g = g;
                    int32_t ndx = std::abs(next.x_fp - nodes[targetId].x_fp), ndy = std::abs(next.y_fp - nodes[targetId].y_fp);
                    next.h = (ndx > ndy ? ndx : ndy) + ((Config::SQRT2_MINUS_1_FP * (ndx < ndy ? ndx : ndy)) >> 10);
                    next.parent = cId; next.f = next.g + next.h;
                    if (next.state == 0) { next.state = 1; m_open.Push(nId); } else m_open.DecreaseKey(nId);
                }
            }
        }
        return res;
    }
private:
    inline void InitNode(CityIntersection& n) { if (n.searchEpoch != m_epoch) { n.searchEpoch = m_epoch; n.state = 0; } }
};
} // namespace halo::urban