/**
 * ============================================================================
 *  H.A.L.O.* — Hardware SIMD Acceleration Engine (ARM NEON / SSE4)
 * ============================================================================
 */
#pragma once

// SỬA TẠI ĐÂY: Lùi ra khỏi folder core để tìm vào folder utils
#include "../utils/halo_types.h" 

#if defined(__ARM_NEON) || defined(__aarch64__)
    #include <arm_neon.h>
    #define HALO_USE_NEON
#elif defined(__SSE4_1__) || defined(_M_IX86) || defined(_M_X64)
    #include <smmintrin.h>
    #define HALO_USE_SSE
#endif

namespace halo::simd {

    // Ép 10 lớp 64-bit thành 1 bản đồ rủi ro duy nhất bằng SIMD NEON
    inline uint64_t Or10Layers64(const uint64_t* layers) noexcept {
#ifdef HALO_USE_NEON
        // Nạp 10 lớp 64-bit vào các thanh ghi 128-bit
        uint64x2_t v01 = vld1q_u64(&layers[0]); 
        uint64x2_t v23 = vld1q_u64(&layers[2]); 
        uint64x2_t v45 = vld1q_u64(&layers[4]); 
        uint64x2_t v67 = vld1q_u64(&layers[6]); 
        uint64x2_t v89 = vld1q_u64(&layers[8]); 

        // OR song song toàn bộ
        uint64x2_t res = vorrq_u64(v01, v23);
        res = vorrq_u64(res, v45);
        res = vorrq_u64(res, v67);
        res = vorrq_u64(res, v89);

        // Hợp nhất kết quả
        return vgetq_lane_u64(res, 0) | vgetq_lane_u64(res, 1);
#else
        return layers[0] | layers[1] | layers[2] | layers[3] | layers[4] | 
               layers[5] | layers[6] | layers[7] | layers[8] | layers[9];
#endif
    }

    // Giữ nguyên hàm tính Heuristic cũ của bồ
    inline void CalculateHeuristics4x(
        const int32_t* targetX, const int32_t* targetY, 
        const int32_t* nodeX_4, const int32_t* nodeY_4, 
        int32_t* outH_4) noexcept 
    {
#ifdef HALO_USE_NEON
        int32x4_t tX = vdupq_n_s32(*targetX);
        int32x4_t tY = vdupq_n_s32(*targetY);
        int32x4_t nX = vld1q_s32(nodeX_4);
        int32x4_t nY = vld1q_s32(nodeY_4);

        int32x4_t dx = vabsq_s32(vsubq_s32(nX, tX));
        int32x4_t dy = vabsq_s32(vsubq_s32(nY, tY));

        int32x4_t mx = vmaxq_s32(dx, dy);
        int32x4_t mn = vminq_s32(dx, dy);

        int32x4_t mx_shifted = vshlq_n_s32(mx, 10);
        int32x4_t mn_mult = vmulq_n_s32(mn, 424); // SQRT2_MINUS_1_FP xấp xỉ 424
        int32x4_t result = vaddq_s32(mx_shifted, mn_mult);

        vst1q_s32(outH_4, result);
#else
        for(int i=0; i<4; ++i) {
            int32_t dx = std::abs(nodeX_4[i] - *targetX);
            int32_t dy = std::abs(nodeY_4[i] - *targetY);
            int32_t mx = dx > dy ? dx : dy;
            int32_t mn = dx < dy ? dx : dy;
            outH_4[i] = (mx << 10) + 424 * mn;
        }
#endif
    }
}