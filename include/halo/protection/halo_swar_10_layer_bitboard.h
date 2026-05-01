/**
 * ============================================================================
 *  H.A.L.O.* AEGIS — 10-LAYER OMNI-SWAR BITBOARD (ULTIMATE EDITION)
 * ============================================================================
 *  Lõi 10 tầng! Ép 10 bản đồ rủi ro thành 1 thanh ghi duy nhất.
 * ============================================================================
 */
#pragma once

// Sửa đường dẫn để nó tìm thấy file types dù bồ đang ở folder protection
#include "../utils/halo_types.h" 
#include <bit>
#include <cstdint>
#include <cstring>

namespace halo::swar {

// 10 TẦNG SINH TỒN - Phân loại chi tiết mọi thứ trên đời!
enum class Layer : uint8_t {
    STATIC_WALLS     = 0, // ⛰️ Núi đá, tòa nhà
    POWER_LINES      = 1, // ⚡ Dây điện cao thế
    WATER_BODIES     = 2, // 💧 Sông hồ
    BROADCAST_TOWERS = 3, // 📡 Trạm phát sóng
    BALLISTIC        = 4, // ☄️ Đạn, súng sơn
    AVIAN_WILDLIFE   = 5, // 🦅 Đại bàng, chim chóc
    AIRCRAFT         = 6, // ✈️ Máy bay, trực thăng
    HUMANS           = 7, // 🚶 Người đi bộ
    VEHICLES         = 8, // 🚗 Xe cộ
    SWARM_ALLIES     = 9, // 🚁 Drone đồng đội
    MAX_LAYERS       = 10
};

class UltimateBitboard64 {
private:
    uint64_t m_rows[static_cast<uint8_t>(Layer::MAX_LAYERS)][64];
    uint64_t m_cols[static_cast<uint8_t>(Layer::MAX_LAYERS)][64];
    int32_t m_w, m_h;

public:
    void Init(int32_t w, int32_t h) noexcept {
        m_w = (w > 64) ? 64 : w; 
        m_h = (h > 64) ? 64 : h;
        ClearAllLayers();
        // Xây tường ranh giới ở lớp tĩnh (Layer 0)
        for(int i = 0; i < m_w; ++i) { SetBit(Layer::STATIC_WALLS, i, 0); SetBit(Layer::STATIC_WALLS, i, m_h-1); }
        for(int i = 0; i < m_h; ++i) { SetBit(Layer::STATIC_WALLS, 0, i); SetBit(Layer::STATIC_WALLS, m_w-1, i); }
    }

    // Đưa hàm này ra PUBLIC để main.cpp có thể vẽ bản đồ kiểm tra
    inline void SetBit(Layer layer, int32_t x, int32_t y) noexcept {
        if(x < 0 || x >= 64 || y < 0 || y >= 64) return; 
        uint8_t l = static_cast<uint8_t>(layer);
        m_rows[l][y] |= (1ULL << x);
        m_cols[l][x] |= (1ULL << y);
    }

    // Đưa hàm này ra PUBLIC để robot có thể kiểm tra từng ô
    inline bool IsBitSet(Layer layer, int32_t x, int32_t y) const noexcept {
        if(x < 0 || x >= 64 || y < 0 || y >= 64) return false;
        return (m_rows[static_cast<uint8_t>(layer)][y] & (1ULL << x)) != 0;
    }

    void ClearAllLayers() noexcept {
        std::memset(m_rows, 0, sizeof(m_rows));
        std::memset(m_cols, 0, sizeof(m_cols));
    }

    // ========================================================================
    // 💥 CẤM THUẬT: ÉP 10 TẦNG THÀNH 1 TIA LASER TRONG VÀI CHU KỲ MÁY
    // ========================================================================
    inline uint64_t GetCompositeRow(int32_t y) const noexcept {
        return m_rows[0][y] | m_rows[1][y] | m_rows[2][y] | m_rows[3][y] | m_rows[4][y] | 
               m_rows[5][y] | m_rows[6][y] | m_rows[7][y] | m_rows[8][y] | m_rows[9][y];
    }

    inline uint64_t GetCompositeCol(int32_t x) const noexcept {
        return m_cols[0][x] | m_cols[1][x] | m_cols[2][x] | m_cols[3][x] | m_cols[4][x] | 
               m_cols[5][x] | m_cols[6][x] | m_cols[7][x] | m_cols[8][x] | m_cols[9][x];
    }

    inline int32_t RaycastEast(int32_t x, int32_t y) const noexcept {
        uint64_t forward = GetCompositeRow(y) & ~((1ULL << (x + 1)) - 1);
        return forward == 0 ? (m_w - 1) : std::countr_zero(forward);
    }

    inline int32_t RaycastWest(int32_t x, int32_t y) const noexcept {
        uint64_t backward = GetCompositeRow(y) & ((1ULL << x) - 1);
        return backward == 0 ? 0 : 63 - std::countl_zero(backward);
    }
};

} // namespace halo::swar