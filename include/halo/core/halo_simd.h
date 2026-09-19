#pragma once

#include "../utils/halo_types.h"
#include <cstdint>

#if defined(__ARM_NEON) || defined(__aarch64__)
#include <arm_neon.h>
#define HALO_SIMD_NEON 1
#elif defined(__AVX512F__)
#include <immintrin.h>
#define HALO_SIMD_AVX512 1
#elif defined(__AVX2__)
#include <immintrin.h>
#define HALO_SIMD_AVX2 1
#elif defined(__SSE4_1__) || defined(__SSE2__)
#include <immintrin.h>
#define HALO_SIMD_SSE 1
#endif

namespace halo::simd {

// Zero-Spill 16-Layer Reduction into Single 64-bit Collision Register
[[nodiscard]] HALO_INLINE uint64_t
Collapse16LayersToShadow(const uint64_t *HALO_RESTRICT layers) noexcept {
#if defined(HALO_SIMD_NEON)
  HALO_PREFETCH(layers);
  HALO_PREFETCH(layers + 8);

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

#elif defined(HALO_SIMD_AVX512)
  __m512i v0 = _mm512_loadu_si512(reinterpret_cast<const void *>(layers));
  __m512i v1 = _mm512_loadu_si512(reinterpret_cast<const void *>(layers + 8));
  __m512i combined = _mm512_or_si512(v0, v1);
  return static_cast<uint64_t>(_mm512_reduce_or_epi64(combined));

#elif defined(HALO_SIMD_AVX2)
  __m256i v0 = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(layers));
  __m256i v1 = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(layers + 4));
  __m256i v2 = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(layers + 8));
  __m256i v3 = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(layers + 12));

  __m256i or01 = _mm256_or_si256(v0, v1);
  __m256i or23 = _mm256_or_si256(v2, v3);
  __m256i orAll = _mm256_or_si256(or01, or23);

  __m128i low128 = _mm256_castsi256_si128(orAll);
  __m128i high128 = _mm256_extracti128_si256(orAll, 1);
  __m128i or128 = _mm_or_si128(low128, high128);

  uint64_t r0 = static_cast<uint64_t>(_mm_cvtsi128_si64(or128));
  uint64_t r1 = static_cast<uint64_t>(_mm_extract_epi64(or128, 1));
  return r0 | r1;

#elif defined(HALO_SIMD_SSE)
  __m128i r0 = _mm_loadu_si128(reinterpret_cast<const __m128i *>(layers));
  __m128i r1 = _mm_loadu_si128(reinterpret_cast<const __m128i *>(layers + 2));
  __m128i r2 = _mm_loadu_si128(reinterpret_cast<const __m128i *>(layers + 4));
  __m128i r3 = _mm_loadu_si128(reinterpret_cast<const __m128i *>(layers + 6));
  __m128i r4 = _mm_loadu_si128(reinterpret_cast<const __m128i *>(layers + 8));
  __m128i r5 = _mm_loadu_si128(reinterpret_cast<const __m128i *>(layers + 10));
  __m128i r6 = _mm_loadu_si128(reinterpret_cast<const __m128i *>(layers + 12));
  __m128i r7 = _mm_loadu_si128(reinterpret_cast<const __m128i *>(layers + 14));

  __m128i a = _mm_or_si128(_mm_or_si128(r0, r1), _mm_or_si128(r2, r3));
  __m128i b = _mm_or_si128(_mm_or_si128(r4, r5), _mm_or_si128(r6, r7));
  __m128i shadow = _mm_or_si128(a, b);

  uint64_t lo = static_cast<uint64_t>(_mm_cvtsi128_si64(shadow));
  uint64_t hi = static_cast<uint64_t>(_mm_extract_epi64(shadow, 1));
  return lo | hi;

#else
  uint64_t shadow = 0;
  for (int i = 0; i < 16; ++i) {
    shadow |= layers[i];
  }
  return shadow;
#endif
}

// Zero-Spill 10-Layer Dedicated Register Collapse
// Directly loads 10 64-bit layers with zero stack spills and branchless vector reduction
[[nodiscard]] HALO_INLINE uint64_t
Collapse10LayersToShadow(const uint64_t *HALO_RESTRICT layers) noexcept {
#if defined(HALO_SIMD_NEON)
  HALO_PREFETCH(layers);
  uint64x2x4_t bA = vld1q_u64_x4(layers);      // Loads layers 0..7
  uint64x2_t bB = vld1q_u64(layers + 8);        // Loads layers 8..9

  uint64x2_t r01 = vorrq_u64(bA.val[0], bA.val[1]);
  uint64x2_t r23 = vorrq_u64(bA.val[2], bA.val[3]);
  uint64x2_t r03 = vorrq_u64(r01, r23);
  uint64x2_t shadow128 = vorrq_u64(r03, bB);

  return vgetq_lane_u64(shadow128, 0) | vgetq_lane_u64(shadow128, 1);

#elif defined(HALO_SIMD_AVX2)
  __m256i v0 = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(layers));     // 0..3
  __m256i v1 = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(layers + 4)); // 4..7
  __m128i v2 = _mm_loadu_si128(reinterpret_cast<const __m128i *>(layers + 8));    // 8..9

  __m256i or01 = _mm256_or_si256(v0, v1);
  __m128i low128 = _mm256_castsi256_si128(or01);
  __m128i high128 = _mm256_extracti128_si256(or01, 1);
  __m128i or128 = _mm_or_si128(_mm_or_si128(low128, high128), v2);

  uint64_t r0 = static_cast<uint64_t>(_mm_cvtsi128_si64(or128));
  uint64_t r1 = static_cast<uint64_t>(_mm_extract_epi64(or128, 1));
  return r0 | r1;

#else
  return layers[0] | layers[1] | layers[2] | layers[3] | layers[4] |
         layers[5] | layers[6] | layers[7] | layers[8] | layers[9];
#endif
}

[[nodiscard]] HALO_INLINE uint64_t
Collapse16LayersToShadow_ARM64(const uint64_t *HALO_RESTRICT layers) noexcept {
  return Collapse16LayersToShadow(layers);
}

[[nodiscard]] HALO_INLINE int32_t CountTrailingZeros64(uint64_t v) noexcept {
  return halo::bits::CountTrailingZeros(v);
}

[[nodiscard]] HALO_INLINE int32_t CountLeadingZeros64(uint64_t v) noexcept {
  return halo::bits::CountLeadingZeros(v);
}

[[nodiscard]] HALO_INLINE int32_t PopCount64(uint64_t v) noexcept {
  return halo::bits::PopCount(v);
}

} // namespace halo::simd