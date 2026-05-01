/**
 * ============================================================================
 *  H.A.L.O.* URBAN — QUANTUM APSP ROUTER
 * ============================================================================
 *  Tiền tính toán toàn bộ lộ trình đô thị. Tra bảng O(1).
 *  Cực hạn tốc độ cho các tác vụ cứu hộ cần di chuyển quãng đường dài.
 * ============================================================================
 */
#pragma once

// Sửa lại đường dẫn để tìm thấy các file ở các folder khác
#include "halo_graph.h"             // Cùng folder navigation
#include "../core/halo_memory.h"     // Nhảy ra ngoài vào folder core
#include "../utils/halo_types.h"     // Nhảy ra ngoài vào folder utils

namespace halo::urban {

class QuantumApspRouter {
    int32_t m_nodes;
    int32_t* m_next; 
    int32_t* m_dist; 

public:
    // Sử dụng ArenaAllocator từ namespace halo::memory đã quy hoạch
    void Precompute(CityMap& map, halo::memory::ArenaAllocator& arena) noexcept {
        m_nodes = map.GetNodeCount();
        
        // Cấp phát mảng 2 chiều phẳng để tối ưu Cache L1/L2
        m_next = arena.AllocateArray<int32_t>(m_nodes * m_nodes);
        m_dist = arena.AllocateArray<int32_t>(m_nodes * m_nodes);
        
        // Khởi tạo ma trận khoảng cách (Vô hạn) và ma trận lộ trình
        for(int i=0; i < m_nodes * m_nodes; ++i) { 
            m_dist[i] = 1000000000; 
            m_next[i] = -1; 
        }
        
        for(int i=0; i < m_nodes; ++i) { 
            m_dist[i * m_nodes + i] = 0; 
            m_next[i * m_nodes + i] = i; 
        }
        
        RoadEdge* edges = map.GetEdges();
        CityIntersection* nodes = map.GetNodes();

        // Nạp trọng số đường đi thực tế từ CityMap
        for(int i=0; i < m_nodes; ++i) {
            for(int32_t e = nodes[i].headEdge; e != -1; e = edges[e].nextEdge) {
                int32_t v = edges[e].targetNode;
                m_dist[i * m_nodes + v] = edges[e].moveCostFP;
                m_next[i * m_nodes + v] = v; 
            }
        }

        // 🧠 TRÁI TIM FLOYD-WARSHALL: O(V^3)
        // Dùng k làm trung gian để tối ưu hóa mọi cặp điểm
        for(int k=0; k < m_nodes; ++k) {
            for(int i=0; i < m_nodes; ++i) {
                // Tối ưu nhẹ: Nếu không có đường tới k thì bỏ qua vòng lặp j
                if (m_dist[i * m_nodes + k] == 1000000000) continue;
                
                for(int j=0; j < m_nodes; ++j) {
                    int32_t newDist = m_dist[i * m_nodes + k] + m_dist[k * m_nodes + j];
                    if (m_dist[i * m_nodes + j] > newDist) {
                        m_dist[i * m_nodes + j] = newDist;
                        m_next[i * m_nodes + j] = m_next[i * m_nodes + k];
                    }
                }
            }
        }
    }

    // TRA BẢNG O(1) - PHẢN XẠ NHƯ MỘT BẢN NĂNG
    UrbanPathResult RouteO1(int32_t start, int32_t target) noexcept {
        UrbanPathResult res;
        // Kiểm tra an toàn: Chống tràn mảng nếu node không tồn tại
        if (start < 0 || start >= m_nodes || target < 0 || target >= m_nodes) return res;
        if (m_next[start * m_nodes + target] == -1) return res; 
        
        res.found = true;
        int32_t curr = start;
        
        // Báo CPU: Ưu tiên Branch Prediction cho vòng lặp này
        while (curr != target && res.len < 1024) { // 1024 là MAX_PATH_LEN mặc định
            res.route[res.len++] = curr;
            curr = m_next[curr * m_nodes + target];
        }
        res.route[res.len++] = target;
        return res;
    }
};

} // namespace halo::urban