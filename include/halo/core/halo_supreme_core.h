/**
 * ============================================================================
 *  H.A.L.O.* SUPREME ENGINE (THE MASTER CORE)
 * ============================================================================
 *  Kiến trúc sư: Nguyên 
 *  Mô tả: Lõi điều phối Tối Thượng. Thâu tóm Memory Arena, Grid JPS+, 
 *  và Urban APSP vào một điểm duy nhất. Bypass hoàn toàn OS Overhead.
 * ============================================================================
 */
#pragma once
#include "halo_types.h"
#include "halo_memory.h"
#include "halo_math.h"
#include "halo_heap.h"
#include "halo_apsp.h"      // Chứa lõi O(1) của bồ
#include "halo_jps_plus.h"  // Chứa lõi Teleport của bồ

namespace halo::core {

class HaloSupremeEngine {
private:
    // 1. TÀI NGUYÊN LÕI (Không xía vô RAM của OS)
    memory::ArenaAllocator m_masterArena;
    
    // 2. CÁC COMPONENT ĐƯỢC ĐIỀU KHIỂN
    Grid* m_grid = nullptr;
    urban::CityMap* m_cityMap = nullptr;
    
    PathNode* m_gridNodes = nullptr;
    BinaryMinHeap m_gridHeap;
    JpsPlusEngine m_jpsEngine;
    urban::QuantumApspRouter m_apspRouter;

    // 3. ĐỒNG HỒ LƯỢNG TỬ (Z.C.R)
    uint32_t m_gridEpoch = 0;

public:
    // ========================================================================
    // [BOOT SEQUENCE] - KHỞI ĐỘNG HỆ THỐNG
    // Gom hết tinh túy khởi tạo vào 1 nút bấm duy nhất!
    // ========================================================================
    void BootSystem(Grid* grid, urban::CityMap* cityMap, size_t megaBytesRAM) {
        // Ép OS nôn ra 1 cục RAM khổng lồ, sau đó khóa cửa lại tự xử!
        m_masterArena.Init(megaBytesRAM * 1024 * 1024);

        m_grid = grid;
        m_cityMap = cityMap;

        // Bơm năng lượng cho Hệ thống Lưới (Maze)
        if (m_grid) {
            int32_t gridTot = m_grid->Size();
            m_gridNodes = m_masterArena.AllocateArray<PathNode>(gridTot);
            std::memset(m_gridNodes, 0, gridTot * sizeof(PathNode));
            m_gridHeap.Init(gridTot, m_gridNodes, m_masterArena);
            
            // Kích hoạt Bảng Cửu Chương Không Gian JPS+
            m_jpsEngine.Precompute(m_grid, m_masterArena);
        }

        // Bơm năng lượng cho Hệ thống Đô Thị (Urban)
        if (m_cityMap) {
            // Nung chảy bản đồ thành Ma trận O(1)
            m_apspRouter.Precompute(*m_cityMap, m_masterArena);
        }
    }

    // ========================================================================
    // [GRID ROUTING] - ĐIỀU KHIỂN JPS+ & TIE-BREAKER (DÀNH CHO MAZE)
    // ========================================================================
    PathResult RouteGrid(Vec2i start, Vec2i target) noexcept {
        PathResult res;
        if (UNLIKELY(!m_grid || !m_grid->InBounds(start) || !m_grid->InBounds(target) || 
                     !m_grid->IsWalkable(m_grid->ToIndex(start)) || !m_grid->IsWalkable(m_grid->ToIndex(target)))) 
            return res;

        if (UNLIKELY(start == target)) { res.found = true; return res; }

        ++m_gridEpoch; // Z.C.R - Đặt lại toàn bản đồ trong 0 nanosecond
        m_gridHeap.Clear();
        int32_t startDist = math::OctileDistanceFP_TieBreak(start, target, start);
        int32_t bestF = 2000000000;
        int32_t sIdx = m_grid->ToIndex(start);

        InitGridNode(sIdx);
        m_gridNodes[sIdx] = {0, startDist, startDist, 0, -1, sIdx, m_gridEpoch, 1, {}};
        m_gridHeap.Push(sIdx);

        while (LIKELY(!m_gridHeap.Empty())) {
            int32_t cIdx = m_gridHeap.Pop();
            PathNode& cNode = m_gridNodes[cIdx];
            cNode.state = 2;
            res.expanded++;

            if (UNLIKELY(cNode.f >= bestF)) continue;

            Vec2i cPos = m_grid->ToVec(cIdx);
            if (UNLIKELY(cPos == target)) {
                res.found = true; res.cost = cNode.g; bestF = std::min(bestF, cNode.g);
                ExtractGridPath(cIdx, res);
                return res;
            }

            bool valid = false;
            
            // Ép luồng SIMD Unroll cho 8 hướng
            #pragma clang loop unroll(full)
            for (int i = 0; i < 8; ++i) {
                // HỎI JPS+ XEM CÓ THỂ DỊCH CHUYỂN (TELEPORT) KHÔNG?
                int32_t jumpSteps = m_jpsEngine.GetJumpDistance(cIdx, i);
                if (jumpSteps == 0) continue; // Hướng này đâm đầu vào tường ngay lập tức

                Vec2i nPos = cPos + Vec2i(Direction::Offsets[i].x * jumpSteps, Direction::Offsets[i].y * jumpSteps);
                int32_t nIdx = m_grid->ToIndex(nPos);
                
                HALO_PREFETCH(&m_gridNodes[nIdx]); // M-Series L1 Cache buster
                
                InitGridNode(nIdx);
                if (UNLIKELY(m_gridNodes[nIdx].state == 2)) continue;

                valid = true;
                PathNode& nNode = m_gridNodes[nIdx];
                
                // Tính Cost thẳng cho cú Teleport
                int32_t g = cNode.g + (jumpSteps * Direction::CostFP[i]);

                if (LIKELY(nNode.state == 0 || g < nNode.g)) {
                    // Căng dây Vector siêu chuẩn xác
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
            if (UNLIKELY(!valid && cNode.parent != -1)) m_grid->AddPenalty(cIdx, Config::DEAD_END_PENALTY);
        }
        return res;
    }

    // ========================================================================
    // [GRAPH ROUTING] - BẬT LÕI LƯỢNG TỬ O(1) CHO ĐÔ THỊ
    // ========================================================================
    urban::UrbanPathResult RouteUrban(int32_t startNode, int32_t targetNode) noexcept {
        // Chỉ gọi sang APSP Component, giải quyết trong vỏn vẹn vài nanoseconds!
        if (UNLIKELY(!m_cityMap)) return {};
        return m_apspRouter.RouteO1(startNode, targetNode);
    }

    // ========================================================================
    // QUẢN LÝ TIẾN TRÌNH VÀ MEMORY (DỌN DẸP O(1))
    // ========================================================================
    void FlushMemory() noexcept {
        // Xả toàn bộ bộ nhớ của cả Grid lẫn Graph về 0 tốn 1 chu kỳ máy
        m_masterArena.Reset();
    }

private:
    inline void InitGridNode(int32_t i) noexcept {
        if (UNLIKELY(m_gridNodes[i].searchEpoch != m_gridEpoch)) {
            m_gridNodes[i].searchEpoch = m_gridEpoch;
            m_gridNodes[i].state = 0;
        }
    }

    inline void ExtractGridPath(int32_t idx, PathResult& res) noexcept {
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