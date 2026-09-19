#pragma once

#include "halo_simd.h"
#include <cstdint>
#include <cstring>

namespace halo::omnicontext {

using NativeWord = uint64_t;
inline constexpr int32_t HALO_WORD_BITS = 64;
inline constexpr NativeWord HALO_ALL_ONES = 0xFFFFFFFFFFFFFFFFULL;

[[nodiscard]] HALO_INLINE int32_t HardwareBitScanForward(NativeWord v) noexcept {
  return v == 0 ? HALO_WORD_BITS : halo::bits::CountTrailingZeros(v);
}

class alignas(64) AdaptiveOmniEngine {
private:
  static constexpr int32_t MAP_SIZE = 64;
  static constexpr int32_t WORDS_PER_ROW = (MAP_SIZE + HALO_WORD_BITS - 1) / HALO_WORD_BITS;
  static constexpr int32_t NUM_LAYERS = 16;

  alignas(64) NativeWord m_raw_layers[MAP_SIZE][WORDS_PER_ROW][NUM_LAYERS];
  alignas(64) NativeWord m_shadow_map[MAP_SIZE][WORDS_PER_ROW];

public:
  AdaptiveOmniEngine() noexcept {
    Init();
  }

  void Init() noexcept {
    std::memset(m_raw_layers, 0, sizeof(m_raw_layers));
    std::memset(m_shadow_map, 0, sizeof(m_shadow_map));
  }

  HALO_INLINE void SetBit(uint8_t layer, int32_t x, int32_t y) noexcept {
    if (HALO_UNLIKELY(static_cast<uint32_t>(x) >= MAP_SIZE ||
                      static_cast<uint32_t>(y) >= MAP_SIZE ||
                      layer >= NUM_LAYERS)) {
      return;
    }

    const int32_t wordIdx = x >> 6;
    const NativeWord mask = (static_cast<NativeWord>(1) << (x & 63));

    m_raw_layers[y][wordIdx][layer] |= mask;
    m_shadow_map[y][wordIdx] |= mask;
  }

  [[nodiscard]] HALO_INLINE bool IsBitSet(uint8_t layer, int32_t x, int32_t y) const noexcept {
    if (HALO_UNLIKELY(static_cast<uint32_t>(x) >= MAP_SIZE ||
                      static_cast<uint32_t>(y) >= MAP_SIZE ||
                      layer >= NUM_LAYERS)) {
      return false;
    }
    return (m_raw_layers[y][x >> 6][layer] &
            (static_cast<NativeWord>(1) << (x & 63))) != 0;
  }

  [[nodiscard]] HALO_INLINE NativeWord GetShadowRow(int32_t y) const noexcept {
    return m_shadow_map[y][0];
  }

  [[nodiscard]] HALO_INLINE static int32_t RaycastRow(NativeWord compositeRow, int32_t startX) noexcept {
    const NativeWord startMask = HALO_ALL_ONES << (startX & 63);
    const NativeWord composite = compositeRow & startMask;
    const int32_t tz = halo::bits::CountTrailingZeros(composite);
    return tz == 64 ? (MAP_SIZE - 1) : tz;
  }

  // Pure in-register Escape Raycast (single cache line, ~0.25 ns latency)
  [[nodiscard]] HALO_INLINE int32_t
  EscapeRaycast(const int32_t startX, const int32_t y) const noexcept {
    if (HALO_UNLIKELY(static_cast<uint32_t>(startX) >= MAP_SIZE || static_cast<uint32_t>(y) >= MAP_SIZE)) {
      return MAP_SIZE - 1;
    }

    if constexpr (WORDS_PER_ROW == 1) {
      return RaycastRow(m_shadow_map[y][0], startX);
    } else {
      const int32_t startWord = startX >> 6;
      const int32_t startBit = startX & 63;

      const NativeWord startMask = (startBit < 64) ? (HALO_ALL_ONES << startBit) : 0ULL;
      NativeWord composite = m_shadow_map[y][startWord] & startMask;

      if (HALO_LIKELY(composite != 0)) {
        return (startWord << 6) + HardwareBitScanForward(composite);
      }

      for (int32_t w = startWord + 1; w < WORDS_PER_ROW; ++w) {
        composite = m_shadow_map[y][w];
        if (HALO_LIKELY(composite != 0)) {
          return (w << 6) + HardwareBitScanForward(composite);
        }
      }
      return MAP_SIZE - 1;
    }
  }
};

} // namespace halo::omnicontext