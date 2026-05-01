/**
 * ============================================================================
 *  H.A.L.O.* QUANTUM — Jump Point Search PLUS (JPS+) Engine
 * ============================================================================
 *  Precomputes jump distances in all 8 directions for O(1) Teleportation.
 * ============================================================================
 */
#pragma once

// Sửa đường dẫn để nó tìm đúng file types và memory dù đang ở trong folder navigation
#include "../utils/halo_types.h"
#include "../core/halo_memory.h"
#include <iostream>
#include <cstring>

namespace halo {

class JpsPlusEngine {
    int32_t* m_jumpTable = nullptr; // Mảng 1D lưu khoảng cách nhảy (N * 8 hướng)
    Grid* m_grid = nullptr;

public:
    void Precompute(Grid* grid, memory::ArenaAllocator& arena) noexcept {
        m_grid = grid;
        int32_t size = grid->Size();
        // Mỗi ô tốn 8 số nguyên (int32_t) cho 8 hướng: East, NE, North, NW, West, SW, South, SE
        m_jumpTable = arena.AllocateArray<int32_t>(size * 8); 
        std::memset(m_jumpTable, 0, size * 8 * sizeof(int32_t));

        std::cout << "⚡ [JPS+ ENGINE]: Đang rèn Bảng Cửu Chương Không Gian...\n";

        int32_t w = grid->Width();
        int32_t h = grid->Size() / w;

        // Quét TRÁI -> PHẢI & TRÊN -> DƯỚI (Tính toán hướng West, North, NorthWest, NorthEast)
        for (int y = 0; y < h; ++y) {
            for (int x = 0; x < w; ++x) {
                int32_t idx = grid->ToIndex(x, y);
                if (!grid->IsWalkable(idx)) continue;

                // West (Hướng 4)
                if (x > 0 && grid->IsWalkable(idx - 1)) m_jumpTable[idx * 8 + 4] = m_jumpTable[(idx - 1) * 8 + 4] + 1;
                // North (Hướng 2)
                if (y > 0 && grid->IsWalkable(idx - w)) m_jumpTable[idx * 8 + 2] = m_jumpTable[(idx - w) * 8 + 2] + 1;
                // NorthWest (Hướng 3)
                if (x > 0 && y > 0 && grid->IsWalkable(idx - w - 1) && grid->IsWalkable(idx - 1) && grid->IsWalkable(idx - w))
                    m_jumpTable[idx * 8 + 3] = m_jumpTable[(idx - w - 1) * 8 + 3] + 1;
                // NorthEast (Hướng 1)
                if (x < w - 1 && y > 0 && grid->IsWalkable(idx - w + 1) && grid->IsWalkable(idx + 1) && grid->IsWalkable(idx - w))
                    m_jumpTable[idx * 8 + 1] = m_jumpTable[(idx - w + 1) * 8 + 1] + 1;
            }
        }

        // Quét PHẢI -> TRÁI & DƯỚI -> TRÊN (Tính toán hướng East, South, SouthEast, SouthWest)
        for (int y = h - 1; y >= 0; --y) {
            for (int x = w - 1; x >= 0; --x) {
                int32_t idx = grid->ToIndex(x, y);
                if (!grid->IsWalkable(idx)) continue;

                // East (Hướng 0)
                if (x < w - 1 && grid->IsWalkable(idx + 1)) m_jumpTable[idx * 8 + 0] = m_jumpTable[(idx + 1) * 8 + 0] + 1;
                // South (Hướng 6)
                if (y < h - 1 && grid->IsWalkable(idx + w)) m_jumpTable[idx * 8 + 6] = m_jumpTable[(idx + w) * 8 + 6] + 1;
                // SouthEast (Hướng 7)
                if (x < w - 1 && y < h - 1 && grid->IsWalkable(idx + w + 1) && grid->IsWalkable(idx + 1) && grid->IsWalkable(idx + w))
                    m_jumpTable[idx * 8 + 7] = m_jumpTable[(idx + w + 1) * 8 + 7] + 1;
                // SouthWest (Hướng 5)
                if (x > 0 && y < h - 1 && grid->IsWalkable(idx + w - 1) && grid->IsWalkable(idx - 1) && grid->IsWalkable(idx + w))
                    m_jumpTable[idx * 8 + 5] = m_jumpTable[(idx + w - 1) * 8 + 5] + 1;
            }
        }
    }

    // LẤY BƯỚC NHẢY O(1) - KHÔNG CẦN VÒNG LẶP WHILE KHI BAY!
    [[nodiscard]] inline int32_t GetJumpDistance(int32_t nodeIdx, int32_t direction) const noexcept {
        return m_jumpTable[nodeIdx * 8 + direction];
    }
};

} // namespace halo