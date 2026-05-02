#pragma once
#include "halo_simd.h"
#include <cstdint>
#include <cstring>

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

[[nodiscard]] __attribute__((always_inline, const)) inline int32_t
HardwareBitScanForward(NativeWord v) noexcept {
#if defined(__aarch64__)

  NativeWord result;
  __asm__ volatile("rbit %0, %1\n\t"
                   "clz  %0, %0\n\t"
                   : "=r"(result)
                   : "r"(v));
  return static_cast<int32_t>(result);
#elif defined(__GNUC__) || defined(__clang__)

  return v == 0 ? HALO_WORD_BITS : __builtin_ctzll(v);
#else

  static const uint8_t DeBruijnBitPosition[64] = {
      0,  1,  2,  53, 3,  7,  54, 27, 4,  38, 41, 8,  34, 55, 48, 28,
      62, 5,  39, 46, 44, 42, 22, 9,  24, 35, 59, 56, 49, 18, 29, 11,
      63, 52, 6,  26, 37, 40, 33, 47, 61, 45, 43, 21, 23, 58, 17, 10,
      51, 25, 36, 32, 60, 20, 57, 16, 50, 31, 19, 15, 30, 14, 13, 12};
  return v == 0 ? HALO_WORD_BITS
                : DeBruijnBitPosition
                      [((uint64_t)((v & -v) * 0x022FDD63CC95386DULL)) >> 58];
#endif
}

class AdaptiveOmniEngine {
private:
  static constexpr int32_t MAP_SIZE = 64;
  static constexpr int32_t WORDS_PER_ROW =
      (MAP_SIZE + HALO_WORD_BITS - 1) / HALO_WORD_BITS;
  static constexpr int32_t NUM_LAYERS = 16;

  alignas(128) NativeWord m_raw_layers[MAP_SIZE][WORDS_PER_ROW][NUM_LAYERS];

  alignas(64) NativeWord m_shadow_map[MAP_SIZE][WORDS_PER_ROW];

public:
  void Init() noexcept {
    std::memset(m_raw_layers, 0, sizeof(m_raw_layers));
    std::memset(m_shadow_map, 0, sizeof(m_shadow_map));
  }

  inline void SetBit(uint8_t layer, int32_t x, int32_t y) noexcept {

    if (__builtin_expect((uint32_t)x >= MAP_SIZE || (uint32_t)y >= MAP_SIZE ||
                             layer >= NUM_LAYERS,
                         0))
      return;

    const int32_t wordIdx = x / HALO_WORD_BITS;
    const NativeWord mask = ((NativeWord)1 << (x % HALO_WORD_BITS));

    m_raw_layers[y][wordIdx][layer] |= mask;

    m_shadow_map[y][wordIdx] |= mask;
  }

  [[nodiscard]] inline bool IsBitSet(uint8_t layer, int32_t x,
                                     int32_t y) const noexcept {
    if ((uint32_t)x >= MAP_SIZE || (uint32_t)y >= MAP_SIZE ||
        layer >= NUM_LAYERS) [[unlikely]]
      return false;
    return (m_raw_layers[y][x / HALO_WORD_BITS][layer] &
            ((NativeWord)1 << (x % HALO_WORD_BITS))) != 0;
  }

  [[nodiscard]] __attribute__((always_inline, hot, pure)) inline int32_t
  EscapeRaycast(const int32_t startX, const int32_t y) const noexcept {
    const int32_t startWord = startX / HALO_WORD_BITS;
    const int32_t startBit = startX % HALO_WORD_BITS;

    const NativeWord startMask = (HALO_ALL_ONES << startBit);

    NativeWord composite = m_shadow_map[y][startWord] & startMask;

    if (__builtin_expect(composite != 0, 1)) {

      return (startWord * HALO_WORD_BITS) + HardwareBitScanForward(composite);
    }

    for (int32_t w = startWord + 1; w < WORDS_PER_ROW; ++w) {
      composite = m_shadow_map[y][w];
      if (__builtin_expect(composite != 0, 1)) {
        return (w * HALO_WORD_BITS) + HardwareBitScanForward(composite);
      }
    }

    return MAP_SIZE - 1;
  }
};

} // namespace halo::omnicontext