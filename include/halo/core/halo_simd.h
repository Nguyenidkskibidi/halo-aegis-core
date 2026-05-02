#pragma once
#include "../utils/halo_types.h"
#include <cstdint>

#if defined(__ARM_NEON) || defined(__aarch64__)
#include <arm_neon.h>
#define HALO_USE_NEON
#endif

namespace halo::simd {

[[nodiscard]] __attribute__((always_inline, hot)) inline uint64_t
Collapse16LayersToShadow_ARM64(const uint64_t *__restrict__ layers) noexcept {
#ifdef HALO_USE_NEON

  __builtin_prefetch(layers, 0, 3);
  __builtin_prefetch(layers + 8, 0, 3);

  uint64x2x4_t blockA = vld1q_u64_x4(layers);
  uint64x2x4_t blockB = vld1q_u64_x4(layers + 8);

  uint64x2_t r0_1 = vorrq_u64(blockA.val[0], blockA.val[1]);
  uint64x2_t r2_3 = vorrq_u64(blockA.val[2], blockA.val[3]);
  uint64x2_t r4_5 = vorrq_u64(blockB.val[0], blockB.val[1]);
  uint64x2_t r6_7 = vorrq_u64(blockB.val[2], blockB.val[3]);

  uint64x2_t halfA = vorrq_u64(r0_1, r2_3);
  uint64x2_t halfB = vorrq_u64(r4_5, r6_7);

  uint64x2_t shadow_128 = vorrq_u64(halfA, halfB);

  return vgetq_lane_u64(shadow_128, 0) | vgetq_lane_u64(shadow_128, 1);
#else

#pragma GCC unroll 16
  uint64_t shadow = 0;
  for (int i = 0; i < 16; ++i)
    shadow |= layers[i];
  return shadow;
#endif
}
} // namespace halo::simd