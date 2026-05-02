#pragma once
#include "../utils/halo_types.h"
#include <cstdint>

#if defined(__ARM_NEON) || defined(__aarch64__)
    #include <arm_neon.h>
    #define HALO_USE_NEON
#endif

namespace halo::simd {

    // =================================================================================
    // 💥 CẤM THUẬT CẤP ĐỘ 1: VECTOR HÓA SIÊU HƯỚNG (SUPERSCALAR VECTORIZATION)
    // Phá vỡ giới hạn băng thông L1 Cache bằng cách hút 128-byte dữ liệu cùng lúc
    // =================================================================================
    
    [[nodiscard]] __attribute__((always_inline, hot))
    inline uint64_t Collapse16LayersToShadow_ARM64(const uint64_t* __restrict__ layers) noexcept {
#ifdef HALO_USE_NEON
        // Dùng __builtin_prefetch để nhắc L1 Cache dọn đường trước 2 chu kỳ máy
        __builtin_prefetch(layers, 0, 3);
        __builtin_prefetch(layers + 8, 0, 3);

        // HÚT LỖ ĐEN LƯỢNG TỬ: Kéo toàn bộ 16 layers (128 bytes) vào 8 thanh ghi 128-bit
        // Lệnh vld1q_u64_x4 là lệnh tải bộ nhớ mạnh nhất của kiến trúc ARMv8
        uint64x2x4_t blockA = vld1q_u64_x4(layers);      // Tải Layer 0 -> 7
        uint64x2x4_t blockB = vld1q_u64_x4(layers + 8);  // Tải Layer 8 -> 15 (Layer 15 là Phantom padding)

        // 🧠 PIPELINE INTERLEAVING: Trộn các lệnh OR để tránh "Data Hazard" (xung đột dữ liệu)
        // Chip M1 có 4 đơn vị ALU NEON, ta vắt kiệt cả 4 cùng một lúc!
        
        // Tầng 1: Ép 8 vector thành 4 (Chạy mất đúng 1 clock cycle trên M1)
        uint64x2_t r0_1 = vorrq_u64(blockA.val[0], blockA.val[1]);
        uint64x2_t r2_3 = vorrq_u64(blockA.val[2], blockA.val[3]);
        uint64x2_t r4_5 = vorrq_u64(blockB.val[0], blockB.val[1]);
        uint64x2_t r6_7 = vorrq_u64(blockB.val[2], blockB.val[3]);

        // Tầng 2: Ép 4 vector thành 2
        uint64x2_t halfA = vorrq_u64(r0_1, r2_3);
        uint64x2_t halfB = vorrq_u64(r4_5, r6_7);

        // Tầng 3: Dung hợp tối thượng thành 1 vector 128-bit
        uint64x2_t shadow_128 = vorrq_u64(halfA, halfB);

        // Tầng 4: Chiết xuất ra 64-bit Word cuối cùng (Rút nửa cao OR với nửa thấp)
        return vgetq_lane_u64(shadow_128, 0) | vgetq_lane_u64(shadow_128, 1);
#else
        // Fallback x86_64 siêu unroll cho các hệ thống không có NEON
        #pragma GCC unroll 16
        uint64_t shadow = 0;
        for (int i = 0; i < 16; ++i) shadow |= layers[i];
        return shadow;
#endif
    }
} // namespace halo::simd