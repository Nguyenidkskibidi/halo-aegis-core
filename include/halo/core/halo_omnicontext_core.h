/**
 * ============================================================================
 *  H.A.L.O.* AEGIS OMNI-HARDWARE (EXTREME RESCUE EDITION)
 * ============================================================================
 *  Kiến trúc sư: Nguyên | Tối ưu: Cache-Line Interleaving & SIMD NEON
 * ============================================================================
 */
#pragma once
#include <cstdint>
#include <cstring>
#include "halo_simd.h" // ĐÃ SỬA: Gọi trực tiếp vì nằm cùng folder core!

namespace halo::omnicontext {

#if defined(__x86_64__) || defined(__aarch64__)
    using NativeWord = uint64_t;
    #define HALO_WORD_BITS 64
    #define HALO_ALL_ONES 0xFFFFFFFFFFFFFFFFULL
#else
    using NativeWord = uint32_t;
    #define HALO_WORD_BITS 32
    #define HALO_ALL_ONES 0xFFFFFFFFUL
#endif

// Ma thuật De Bruijn đếm bit 0 cuối cùng (Trailing Zeros)
static const uint8_t DeBruijnBitPosition[32] = {
    0, 1, 28, 2, 29, 14, 24, 3, 30, 22, 20, 15, 25, 17, 4, 8,
    31, 27, 13, 23, 21, 19, 16, 7, 26, 12, 18, 6, 11, 5, 10, 9
};

inline int32_t FastCountTrailingZeros(NativeWord v) noexcept {
    if (v == 0) return HALO_WORD_BITS;
    #if defined(__GNUC__) || defined(__clang__)
        if constexpr (sizeof(NativeWord) == 8) return __builtin_ctzll(v);
        else return __builtin_ctz((unsigned int)v);
    #else
        return DeBruijnBitPosition[((uint32_t)((v & -v) * 0x077CB531U)) >> 27];
    #endif
}

class AdaptiveOmniEngine {
private:
    static constexpr int32_t WORDS_PER_ROW = (64 + HALO_WORD_BITS - 1) / HALO_WORD_BITS;
    
    // CẤU TRÚC: [Y][WORD][10 LAYERS] - Cache-Friendly tối thượng
    alignas(64) NativeWord m_data[64][WORDS_PER_ROW][10];

public:
    void Init() noexcept { std::memset(m_data, 0, sizeof(m_data)); }

    inline bool IsBitSet(uint8_t layer, int32_t x, int32_t y) const noexcept {
        if(x < 0 || x >= 64 || y < 0 || y >= 64 || layer >= 10) return false;
        return (m_data[y][x / HALO_WORD_BITS][layer] & ((NativeWord)1 << (x % HALO_WORD_BITS))) != 0;
    }

    inline void SetBit(uint8_t layer, int32_t x, int32_t y) noexcept {
        if(x < 0 || x >= 64 || y < 0 || y >= 64 || layer >= 10) return;
        m_data[y][x / HALO_WORD_BITS][layer] |= ((NativeWord)1 << (x % HALO_WORD_BITS));
    }

    // PHẢN XẠ CỰC HẠN: Dùng SIMD NEON xử lý 10 lớp trong 1 nốt nhạc
    __attribute__((always_inline))
    inline int32_t EscapeRaycast(int32_t startX, int32_t y) const noexcept {
        const int32_t startWord = startX / HALO_WORD_BITS;
        const int32_t startBit = startX % HALO_WORD_BITS;
        
        for (int32_t w = startWord; w < WORDS_PER_ROW; ++w) {
            // Triệu hồi hàm SIMD từ halo_simd.h
            NativeWord composite = halo::simd::Or10Layers64(m_data[y][w]);
            
            if (w == startWord) {
                composite &= (HALO_ALL_ONES << startBit);
            }

            if (composite != 0) {
                return (w * HALO_WORD_BITS) + FastCountTrailingZeros(composite);
            }
        }
        return 63; 
    }
};

} // namespace halo::omnicontext