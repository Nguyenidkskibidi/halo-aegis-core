/**
 * ============================================================================
 * H.A.L.O.* AEGIS FUSION CORE — Omni-Sensor Interface
 * ============================================================================
 * Kiến trúc sư: Nguyên
 * Mô tả: Ép mọi cảm biến thành Dữ liệu Bitboard (Đã đồng bộ 10-Layer)
 * ============================================================================
 */
#pragma once

#include "../utils/halo_types.h"
#include "halo_swar_10_layer_bitboard.h" // Gọi trực tiếp vì cùng folder protection
#include <cmath>
#include <vector>

namespace halo::aegis {

enum class ThreatType { BALLISTIC_PROJECTILE, EMP_JAMMER_WAVE };

struct Threat {
    ThreatType type;
    Vec2i origin;
    Vec2i velocity;
    int32_t coneWidth;
};

class AegisFusionEngine {
private:
    // SỬA: Dùng đúng tên class UltimateBitboard64 bồ đã gửi
    swar::UltimateBitboard64* m_aegisGrid; 

public:
    void BindGrid(swar::UltimateBitboard64* grid) noexcept {
        m_aegisGrid = grid;
    }

    // ========================================================================
    // 🏹 DÀNH CHO RADAR / CAMERA (Vẽ quỹ đạo đạn bằng thuật toán Bresenham)
    // ========================================================================
    void InjectBallisticThreat(Vec2i startPos, Vec2i velocityVector) noexcept {
        if (!m_aegisGrid) return;

        int32_t x0 = startPos.x; int32_t y0 = startPos.y;
        // Dự đoán quỹ đạo 10 bước tới
        int32_t x1 = startPos.x + velocityVector.x * 10; 
        int32_t y1 = startPos.y + velocityVector.y * 10;

        int32_t dx = std::abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
        int32_t dy = -std::abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
        int32_t err = dx + dy, e2;

        while (true) {
            // SỬA: Gọi đúng Layer::BALLISTIC (ID = 4) theo chuẩn 10 lớp
            m_aegisGrid->SetBit(swar::Layer::BALLISTIC, x0, y0);
            
            if (x0 == x1 && y0 == y1) break;
            e2 = 2 * err;
            if (e2 >= dy) { err += dy; x0 += sx; }
            if (e2 <= dx) { err += dx; y0 += sy; }
        }
    }

    // ========================================================================
    // 📻 DÀNH CHO RF SCANNER (Sóng nhiễu EMP / Vùng cấm bay)
    // ========================================================================
    void InjectEmpWaveThreat(Vec2i origin, int32_t maxRadius) noexcept {
        if (!m_aegisGrid) return;

        for (int r = 0; r < maxRadius; ++r) {
            int32_t currentY = origin.y - r;
            if (currentY < 0) break;
            
            int32_t spread = r / 2; 
            for (int dx = -spread; dx <= spread; ++dx) {
                int32_t currentX = origin.x + dx;
                if (currentX >= 0 && currentX < 64) {
                    // SỬA: Sóng nhiễu trạm phát sóng (ID = 3)
                    m_aegisGrid->SetBit(swar::Layer::BROADCAST_TOWERS, currentX, currentY);
                }
            }
        }
    }
};

} // namespace halo::aegis