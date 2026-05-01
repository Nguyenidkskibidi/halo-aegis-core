#pragma once
#include "../utils/halo_types.h"
#include <cstdint>

#if defined(__ARM_NEON) || defined(__aarch64__)
    #include <arm_neon.h>
    #define HALO_USE_NEON
#endif

namespace halo::simd {
    // 💥 ÉP XUNG LƯỢNG TỬ: DÙNG LỆNH HÚT X4 CỦA ARM NEON
    __attribute__((always_inline))
    inline uint64_t Or16Layers64(const uint64_t* layers) noexcept {
#ifdef HALO_USE_NEON
        // Hút 64 Bytes (8 layers) TRONG ĐÚNG 1 LỆNH PHẦN CỨNG!
        uint64x2x4_t block0 = vld1q_u64_x4(layers);      
        uint64x2x4_t block1 = vld1q_u64_x4(layers + 8);  

        // Rút gọn nhánh 1 (Song song tối đa)
        uint64x2_t r01 = vorrq_u64(block0.val[0], block0.val[1]);
        uint64x2_t r23 = vorrq_u64(block0.val[2], block0.val[3]);
        uint64x2_t half1 = vorrq_u64(r01, r23);

        // Rút gọn nhánh 2 
        uint64x2_t r45 = vorrq_u64(block1.val[0], block1.val[1]);
        uint64x2_t r67 = vorrq_u64(block1.val[2], block1.val[3]);
        uint64x2_t half2 = vorrq_u64(r45, r67);

        // Dung hợp 16 layers
        uint64x2_t final_res = vorrq_u64(half1, half2);

        return vgetq_lane_u64(final_res, 0) | vgetq_lane_u64(final_res, 1);
#else
        return layers[0] | layers[1] | layers[2] | layers[3] | layers[4] | layers[5] | layers[6] | layers[7] |
               layers[8] | layers[9] | layers[10]| layers[11]| layers[12]| layers[13]| layers[14];
#endif
    }
}