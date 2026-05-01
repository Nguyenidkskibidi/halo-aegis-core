/**
 * ============================================================================
 *  H.A.L.O.* AEGIS OMNIVERSE — 10-LAYER BITBOARD ENGINE
 * ============================================================================
 *  Kiến trúc sư: Nguyên
 *  Mô tả: Hệ thống Lưới Bitboard 10 Lớp. Tích hợp vạn vật trong vũ trụ.
 *  Raycasting gộp 10 lớp bằng ILP (Instruction-Level Parallelism).
 * ============================================================================
 */
#pragma once

// TẠI ĐÂY: Sửa lại đường dẫn để tìm thấy halo_types.h trong folder utils
#include "../utils/halo_types.h" 

#include <bit>
#include <cstdint>
#include <cstring>
#include <cmath>

namespace halo::omniverse {

// ĐỦ 10 LỚP THEO YÊU CẦU CỦA KIẾN TRÚC SƯ NGUYÊN!
enum class Layer : uint8_t {
    TERRAIN       = 0,  // Núi đá, cây cối, cao ốc
    BALLISTIC     = 1,  // Đạn súng, súng sơn, tên lửa
    EMP_RADIO     = 2,  // Súng nhiễu sóng, trạm phát sóng mạnh
    POWER_LINES   = 3,  // Dây điện cao thế (khó thấy nhất)
    AVIATION      = 4,  // Máy bay dân dụng/trực thăng
    AVIAN_LIFE    = 5,  // Chim chóc, đại bàng (vật thể bay khó đoán)
    WATER_BODIES  = 6,  // Sông, hồ, mặt nước (rớt là chết)
    HUMANS        = 7,  // Con người (Tuyệt đối giữ khoảng cách an toàn)
    VEHICLES      = 8,  // Xe cộ, cần cẩu đang di chuyển
    SWARM_ALLIES  = 9,  // Drone đồng đội trong bầy đàn
    MAX_LAYERS    = 10
};

class OmniverseEngine {
private:
    uint64_t m_rows[10][64];
    uint64_t m_cols[10][64];
    int32_t m_w = 64, m_h = 64;

public:
    void Init() noexcept {
        std::memset(m_rows, 0, sizeof(m_rows));
        std::memset(m_cols, 0, sizeof(m_cols));
        // Xây tường ranh giới ở lớp địa hình
        for(int i = 0; i < 64; ++i) { SetBit(Layer::TERRAIN, i, 0); SetBit(Layer::TERRAIN, i, 63); }
        for(int i = 0; i < 64; ++i) { SetBit(Layer::TERRAIN, 0, i); SetBit(Layer::TERRAIN, 63, i); }
    }

    inline void SetBit(Layer layer, int32_t x, int32_t y) noexcept {
        if(x < 0 || x >= 64 || y < 0 || y >= 64) return;
        uint8_t l = static_cast<uint8_t>(layer);
        m_rows[l][y] |= (1ULL << x);
        m_cols[l][x] |= (1ULL << y);
    }

    inline bool IsBitSet(Layer layer, int32_t x, int32_t y) const noexcept {
        if(x < 0 || x >= 64 || y < 0 || y >= 64) return false;
        return (m_rows[static_cast<uint8_t>(layer)][y] & (1ULL << x)) != 0;
    }

    // ========================================================================
    // 🧠 CÁC THUẬT TOÁN NHẬN DIỆN CẢM BIẾN (SENSOR FUSION)
    // ========================================================================

    void InjectEagle(Vec2i pos, Vec2i attackVector) noexcept {
        for(int i = 0; i < 8; ++i) {
            SetBit(Layer::AVIAN_LIFE, pos.x + attackVector.x * i, pos.y + attackVector.y * i);
            SetBit(Layer::AVIAN_LIFE, pos.x + attackVector.x * i + 1, pos.y + attackVector.y * i); 
        }
    }

    void InjectAirplane(Vec2i center, int32_t radius) noexcept {
        for(int y = center.y - radius; y <= center.y + radius; ++y) {
            for(int x = center.x - radius; x <= center.x + radius; ++x) {
                SetBit(Layer::AVIATION, x, y);
            }
        }
    }

    void InjectPowerLine(Vec2i p1, Vec2i p2) noexcept {
        int32_t x0 = p1.x, y0 = p1.y, x1 = p2.x, y1 = p2.y;
        int32_t dx = std::abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
        int32_t dy = -std::abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
        int32_t err = dx + dy, e2;
        while (true) {
            SetBit(Layer::POWER_LINES, x0, y0);
            if (x0 == x1 && y0 == y1) break;
            e2 = 2 * err;
            if (e2 >= dy) { err += dy; x0 += sx; }
            if (e2 <= dx) { err += dx; y0 += sy; }
        }
    }

    void InjectHuman(Vec2i pos) noexcept {
        for(int y = pos.y - 2; y <= pos.y + 2; ++y) {
            for(int x = pos.x - 2; x <= pos.x + 2; ++x) {
                SetBit(Layer::HUMANS, x, y);
            }
        }
    }

    // ========================================================================
    // 💥 COMPOSITE RAYCASTING
    // ========================================================================

    inline uint64_t GetCompositeRow(int32_t y) const noexcept {
        return m_rows[0][y] | m_rows[1][y] | m_rows[2][y] | m_rows[3][y] | 
               m_rows[4][y] | m_rows[5][y] | m_rows[6][y] | m_rows[7][y] | 
               m_rows[8][y] | m_rows[9][y];
    }

    inline int32_t RaycastEastMaster(int32_t x, int32_t y) const noexcept {
        uint64_t row = GetCompositeRow(y);
        uint64_t mask = ~((1ULL << (x + 1)) - 1);
        uint64_t forward = row & mask;
        return forward == 0 ? 63 : std::countr_zero(forward);
    }

    inline int32_t RaycastWestMaster(int32_t x, int32_t y) const noexcept {
        uint64_t row = GetCompositeRow(y);
        uint64_t mask = (1ULL << x) - 1;
        uint64_t backward = row & mask;
        return backward == 0 ? 0 : 63 - std::countl_zero(backward);
    }
};

} // namespace halo::omniverse