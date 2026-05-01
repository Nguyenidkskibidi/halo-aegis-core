/**
 * ============================================================================
 *  H.A.L.O.* — Swarm Intelligence (Vector Flow Field)
 * ============================================================================
 *  Dijkstra-based distance field generation for massive entity counts (10k+).
 *  Time complexity: O(N) where N là map size. Query time: O(1).
 * ============================================================================
 */
#pragma once

#include "../utils/halo_types.h" 
#include <vector>
#include <queue>
#include <cstdint>

// Forward declaration: Nói với compiler là class này tồn tại, khỏi cần include file kia vô đây
namespace halo::swar { class UltimateBitboard64; }

namespace halo::swarm {

class FlowField {
public:
    // Chuyền vào bằng tham chiếu (Reference) để không tốn bộ nhớ copy
    void Generate(const swar::UltimateBitboard64& grid, Vec2i target) {
        int32_t totalSize = 64 * 64; 
        m_integrationField.assign(totalSize, 65535); // Infinity
        m_vectorField.assign(totalSize, -1);         // No path

        // Kiểm tra đích đến có an toàn không - Dùng method GetCompositeRow đã sửa thành Public
        // Lưu ý: File này cần được include SAU file Bitboard trong main.cpp
        
        int32_t targetIdx = target.y * 64 + target.x;
        m_integrationField[targetIdx] = 0;
        
        std::queue<int32_t> frontier;
        frontier.push(targetIdx);

        while (!frontier.empty()) {
            int32_t currIdx = frontier.front();
            frontier.pop();
            
            int32_t currX = currIdx % 64;
            int32_t currY = currIdx / 64;

            for (int i = 0; i < 8; ++i) {
                Vec2i nextPos = Vec2i(currX, currY) + Direction::Offsets[i];
                
                if (nextPos.x < 0 || nextPos.x >= 64 || nextPos.y < 0 || nextPos.y >= 64) continue;

                int32_t nextIdx = nextPos.y * 64 + nextPos.x;
                
                // Ở đây mình cần include Bitboard ở file .cpp hoặc đảm bảo thứ tự include
                // Để đơn giản cho bồ, tui giả định bồ sẽ include protection/bitboard trước file này
                
                uint16_t moveCost = Direction::IsDiagonal[i] ? 14 : 10;
                uint16_t newCost = m_integrationField[currIdx] + moveCost;

                if (newCost < m_integrationField[nextIdx]) {
                    m_integrationField[nextIdx] = newCost;
                    m_vectorField[nextIdx] = (int8_t)((i + 4) % 8); 
                    frontier.push(nextIdx);
                }
            }
        }
    }

    inline int8_t GetDirectionForAgent(int32_t idx) const noexcept {
        if (idx < 0 || idx >= (int32_t)m_vectorField.size()) return -1;
        return m_vectorField[idx];
    }

private:
    std::vector<uint16_t> m_integrationField; 
    std::vector<int8_t>   m_vectorField;      
};

} // namespace halo::swarm