#pragma once

#include "../core/halo_memory.h"
#include "../core/halo_simd.h"
#include "../utils/halo_types.h"
#include <cassert>
#include <cstdint>
#include <cstring>

namespace halo::swar {

enum class Layer : uint8_t {
  STATIC_WALLS = 0,
  POWER_LINES = 1,
  WATER_BODIES = 2,
  BROADCAST_TOWERS = 3,
  BALLISTIC = 4,
  AVIAN_WILDLIFE = 5,
  AIRCRAFT = 6,
  HUMANS = 7,
  VEHICLES = 8,
  SWARM_ALLIES = 9,
  MAX_LAYERS = 10
};

// 64x64 Ultimate Bitboard
// Memory is cache-line interleaved: m_rows[Y][Layer] and m_cols[X][Layer]
// Zero-spill in-register 10-layer SIMD evaluation.
class alignas(64) UltimateBitboard64 {
private:
  static constexpr int32_t PADDED_LAYERS = 16;
  alignas(64) uint64_t m_rows[64][PADDED_LAYERS];
  alignas(64) uint64_t m_cols[64][PADDED_LAYERS];
  alignas(64) uint64_t m_shadowRows[64];
  alignas(64) uint64_t m_shadowCols[64];
  int32_t m_w = 64;
  int32_t m_h = 64;

public:
  UltimateBitboard64() noexcept {
    Init(64, 64);
  }

  void Init(int32_t w = 64, int32_t h = 64) noexcept {
    m_w = (w > 64) ? 64 : ((w < 1) ? 1 : w);
    m_h = (h > 64) ? 64 : ((h < 1) ? 1 : h);
    ClearAllLayers();

    // Static boundary walls
    for (int i = 0; i < m_w; ++i) {
      SetBit(Layer::STATIC_WALLS, i, 0);
      SetBit(Layer::STATIC_WALLS, i, m_h - 1);
    }
    for (int i = 0; i < m_h; ++i) {
      SetBit(Layer::STATIC_WALLS, 0, i);
      SetBit(Layer::STATIC_WALLS, m_w - 1, i);
    }
  }

  HALO_INLINE void SetBit(Layer layer, int32_t x, int32_t y) noexcept {
    if (HALO_UNLIKELY(static_cast<uint32_t>(x) >= static_cast<uint32_t>(m_w) ||
                      static_cast<uint32_t>(y) >= static_cast<uint32_t>(m_h))) return;
    const uint8_t l = static_cast<uint8_t>(layer);
    if (HALO_UNLIKELY(l >= static_cast<uint8_t>(Layer::MAX_LAYERS))) return;

    const uint64_t maskX = (1ULL << x);
    const uint64_t maskY = (1ULL << y);

    m_rows[y][l] |= maskX;
    m_cols[x][l] |= maskY;
    m_shadowRows[y] |= maskX;
    m_shadowCols[x] |= maskY;
  }

  HALO_INLINE void ClearBit(Layer layer, int32_t x, int32_t y) noexcept {
    if (HALO_UNLIKELY(static_cast<uint32_t>(x) >= static_cast<uint32_t>(m_w) ||
                      static_cast<uint32_t>(y) >= static_cast<uint32_t>(m_h))) return;
    const uint8_t l = static_cast<uint8_t>(layer);
    if (HALO_UNLIKELY(l >= static_cast<uint8_t>(Layer::MAX_LAYERS))) return;

    const uint64_t maskX = (1ULL << x);
    const uint64_t maskY = (1ULL << y);

    m_rows[y][l] &= ~maskX;
    m_cols[x][l] &= ~maskY;
    m_shadowRows[y] = simd::Collapse10LayersToShadow(m_rows[y]);
    m_shadowCols[x] = simd::Collapse10LayersToShadow(m_cols[x]);
  }

  [[nodiscard]] HALO_INLINE bool IsBitSet(Layer layer, int32_t x, int32_t y) const noexcept {
    if (HALO_UNLIKELY(static_cast<uint32_t>(x) >= static_cast<uint32_t>(m_w) ||
                      static_cast<uint32_t>(y) >= static_cast<uint32_t>(m_h))) return false;
    const uint8_t l = static_cast<uint8_t>(layer);
    if (HALO_UNLIKELY(l >= static_cast<uint8_t>(Layer::MAX_LAYERS))) return false;

    return (m_rows[y][l] & (1ULL << x)) != 0;
  }

  void ClearAllLayers() noexcept {
    std::memset(m_rows, 0, sizeof(m_rows));
    std::memset(m_cols, 0, sizeof(m_cols));
    std::memset(m_shadowRows, 0, sizeof(m_shadowRows));
    std::memset(m_shadowCols, 0, sizeof(m_shadowCols));
  }

  [[nodiscard]] HALO_INLINE uint64_t GetCompositeRow(int32_t y) const noexcept {
    assert(y >= 0 && y < 64);
    return m_shadowRows[y];
  }

  [[nodiscard]] HALO_INLINE uint64_t GetCompositeCol(int32_t x) const noexcept {
    assert(x >= 0 && x < 64);
    return m_shadowCols[x];
  }

  // Branchless, zero-UB Raycasts
  [[nodiscard]] HALO_INLINE int32_t RaycastEast(int32_t x, int32_t y) const noexcept {
    if (HALO_UNLIKELY(x >= m_w - 1)) return m_w - 1;
    const uint64_t row = GetCompositeRow(y);
    const uint64_t mask = (x < 63) ? (~0ULL << (x + 1)) : 0ULL;
    const uint64_t forward = row & mask;
    return forward == 0 ? (m_w - 1) : halo::bits::CountTrailingZeros(forward);
  }

  [[nodiscard]] HALO_INLINE int32_t RaycastWest(int32_t x, int32_t y) const noexcept {
    if (HALO_UNLIKELY(x <= 0)) return 0;
    const uint64_t row = GetCompositeRow(y);
    const uint64_t mask = (x > 0) ? ((1ULL << (x < 64 ? x : 63)) - 1ULL) : 0ULL;
    const uint64_t backward = row & mask;
    return backward == 0 ? 0 : 63 - halo::bits::CountLeadingZeros(backward);
  }

  [[nodiscard]] HALO_INLINE int32_t RaycastSouth(int32_t x, int32_t y) const noexcept {
    if (HALO_UNLIKELY(y >= m_h - 1)) return m_h - 1;
    const uint64_t col = GetCompositeCol(x);
    const uint64_t mask = (y < 63) ? (~0ULL << (y + 1)) : 0ULL;
    const uint64_t forward = col & mask;
    return forward == 0 ? (m_h - 1) : halo::bits::CountTrailingZeros(forward);
  }

  [[nodiscard]] HALO_INLINE int32_t RaycastNorth(int32_t x, int32_t y) const noexcept {
    if (HALO_UNLIKELY(y <= 0)) return 0;
    const uint64_t col = GetCompositeCol(x);
    const uint64_t mask = (y > 0) ? ((1ULL << (y < 64 ? y : 63)) - 1ULL) : 0ULL;
    const uint64_t backward = col & mask;
    return backward == 0 ? 0 : 63 - halo::bits::CountLeadingZeros(backward);
  }

  [[nodiscard]] inline int32_t Width() const noexcept { return m_w; }
  [[nodiscard]] inline int32_t Height() const noexcept { return m_h; }
};

// C++20 NTTP Compile-Time Geometry Hazard Matrix
// Eliminates runtime divisions/multiplications via bitwise shift resolution
template <int32_t W = 0, int32_t H = 0>
class alignas(64) LayeredHazardMatrixT {
private:
  static constexpr int32_t PADDED_LAYERS = 16;
  static constexpr bool HAS_STATIC_DIM = (W > 0 && H > 0);
  static constexpr int32_t STATIC_WORDS_PER_ROW = HAS_STATIC_DIM ? ((W + 63) / 64) : 0;
  static constexpr bool IS_POW2_WORDS = HAS_STATIC_DIM && IsPowerOfTwo(STATIC_WORDS_PER_ROW);
  static constexpr int32_t SHIFT_WORDS = IS_POW2_WORDS ? Log2Constexpr(STATIC_WORDS_PER_ROW) : 0;

  int32_t m_w = W;
  int32_t m_h = H;
  int32_t m_wordsPerRow = STATIC_WORDS_PER_ROW;
  uint64_t *m_data = nullptr;

  [[nodiscard]] HALO_INLINE size_t ComputeOffset(int32_t y, int32_t wordIdx, int32_t layer) const noexcept {
    if constexpr (IS_POW2_WORDS) {
      return static_cast<size_t>(((y << SHIFT_WORDS) | wordIdx) << 4) | layer;
    } else {
      return (static_cast<size_t>(y) * m_wordsPerRow + wordIdx) * PADDED_LAYERS + layer;
    }
  }

public:
  LayeredHazardMatrixT() noexcept = default;

  void Init(int32_t width, int32_t height, memory::ArenaAllocator &arena) noexcept {
    m_w = width;
    m_h = height;
    m_wordsPerRow = (width + 63) / 64;
    const size_t totalSlots = static_cast<size_t>(m_h) * m_wordsPerRow * PADDED_LAYERS;
    m_data = arena.AllocateArray<uint64_t, 64>(totalSlots);
    if (m_data) {
      std::memset(m_data, 0, totalSlots * sizeof(uint64_t));
    }
  }

  HALO_INLINE void SetBit(Layer layer, int32_t x, int32_t y) noexcept {
    if (HALO_UNLIKELY(x < 0 || x >= m_w || y < 0 || y >= m_h || !m_data)) return;
    const uint8_t l = static_cast<uint8_t>(layer);
    if (HALO_UNLIKELY(l >= static_cast<uint8_t>(Layer::MAX_LAYERS))) return;

    const int32_t wordIdx = x >> 6;
    const int32_t bitIdx = x & 63;
    const size_t offset = ComputeOffset(y, wordIdx, l);
    m_data[offset] |= (1ULL << bitIdx);
  }

  HALO_INLINE void ClearBit(Layer layer, int32_t x, int32_t y) noexcept {
    if (HALO_UNLIKELY(x < 0 || x >= m_w || y < 0 || y >= m_h || !m_data)) return;
    const uint8_t l = static_cast<uint8_t>(layer);
    if (HALO_UNLIKELY(l >= static_cast<uint8_t>(Layer::MAX_LAYERS))) return;

    const int32_t wordIdx = x >> 6;
    const int32_t bitIdx = x & 63;
    const size_t offset = ComputeOffset(y, wordIdx, l);
    m_data[offset] &= ~(1ULL << bitIdx);
  }

  [[nodiscard]] HALO_INLINE bool IsBitSet(Layer layer, int32_t x, int32_t y) const noexcept {
    if (HALO_UNLIKELY(x < 0 || x >= m_w || y < 0 || y >= m_h || !m_data)) return false;
    const uint8_t l = static_cast<uint8_t>(layer);
    if (HALO_UNLIKELY(l >= static_cast<uint8_t>(Layer::MAX_LAYERS))) return false;

    const int32_t wordIdx = x >> 6;
    const int32_t bitIdx = x & 63;
    const size_t offset = ComputeOffset(y, wordIdx, l);
    return (m_data[offset] & (1ULL << bitIdx)) != 0;
  }

  [[nodiscard]] HALO_INLINE bool IsBlocked(int32_t x, int32_t y) const noexcept {
    if (HALO_UNLIKELY(x < 0 || x >= m_w || y < 0 || y >= m_h || !m_data)) return true;
    const int32_t wordIdx = x >> 6;
    const int32_t bitIdx = x & 63;
    const uint64_t composite = GetCompositeWord(y, wordIdx);
    return (composite & (1ULL << bitIdx)) != 0;
  }

  [[nodiscard]] HALO_INLINE bool IsHullBlocked(float fx, float fy, float radius = 0.8f) const noexcept {
    int32_t cx = static_cast<int32_t>(std::round(fx));
    int32_t cy = static_cast<int32_t>(std::round(fy));
    float rSq = radius * radius;
    for (int32_t dy = -1; dy <= 1; ++dy) {
      for (int32_t dx = -1; dx <= 1; ++dx) {
        int32_t nx = cx + dx;
        int32_t ny = cy + dy;
        if (IsBlocked(nx, ny)) {
          float ox = static_cast<float>(nx);
          float oy = static_cast<float>(ny);
          float distSq = (fx - ox) * (fx - ox) + (fy - oy) * (fy - oy);
          if (distSq <= rSq) return true;
        }
      }
    }
    return false;
  }

  [[nodiscard]] float RaycastLine(Vec2f start, Vec2f dir, float maxDist) const noexcept {
    if (maxDist <= 0.0f) return 0.0f;
    constexpr float stepSize = 0.7f;
    const int32_t numSteps = static_cast<int32_t>(maxDist / stepSize);
    int32_t lastWordIdx = -1;
    int32_t lastY = -1;
    uint64_t cachedComposite = 0;
    for (int32_t s = 1; s <= numSteps; ++s) {
      Vec2f p = start + dir * (static_cast<float>(s) * stepSize);
      int32_t ix = static_cast<int32_t>(p.x + 0.5f);
      int32_t iy = static_cast<int32_t>(p.y + 0.5f);
      if (HALO_UNLIKELY(ix < 0 || ix >= m_w || iy < 0 || iy >= m_h || !m_data)) {
        return static_cast<float>(s - 1) * stepSize;
      }
      const int32_t wordIdx = ix >> 6;
      if (iy != lastY || wordIdx != lastWordIdx) {
        lastY = iy;
        lastWordIdx = wordIdx;
        cachedComposite = GetCompositeWord(iy, wordIdx);
      }
      if ((cachedComposite & (1ULL << (ix & 63))) != 0) {
        return static_cast<float>(s - 1) * stepSize;
      }
    }
    return maxDist;
  }

  [[nodiscard]] HALO_INLINE uint64_t GetCompositeWord(int32_t y, int32_t wordIdx) const noexcept {
    assert(y >= 0 && y < m_h && wordIdx >= 0 && wordIdx < m_wordsPerRow && m_data);
    const size_t offset = ComputeOffset(y, wordIdx, 0);
    return simd::Collapse10LayersToShadow(&m_data[offset]);
  }

  // Branchless multi-word Raycast East
  [[nodiscard]] int32_t RaycastEast(int32_t startX, int32_t y) const noexcept {
    if (HALO_UNLIKELY(startX >= m_w - 1 || y < 0 || y >= m_h || !m_data)) return m_w - 1;

    const int32_t startWord = startX >> 6;
    const int32_t startBit = startX & 63;

    uint64_t composite = GetCompositeWord(y, startWord);
    const uint64_t mask = (startBit < 63) ? (~0ULL << (startBit + 1)) : 0ULL;
    composite &= mask;

    if (composite != 0) {
      int32_t hitX = (startWord << 6) + halo::bits::CountTrailingZeros(composite);
      return hitX < m_w ? hitX : m_w - 1;
    }

    for (int32_t w = startWord + 1; w < m_wordsPerRow; ++w) {
      composite = GetCompositeWord(y, w);
      if (composite != 0) {
        int32_t hitX = (w << 6) + halo::bits::CountTrailingZeros(composite);
        return hitX < m_w ? hitX : m_w - 1;
      }
    }
    return m_w - 1;
  }

  // Branchless multi-word Raycast West
  [[nodiscard]] int32_t RaycastWest(int32_t startX, int32_t y) const noexcept {
    if (HALO_UNLIKELY(startX <= 0 || y < 0 || y >= m_h || !m_data)) return 0;

    const int32_t startWord = startX >> 6;
    const int32_t startBit = startX & 63;

    uint64_t composite = GetCompositeWord(y, startWord);
    const uint64_t mask = (startBit > 0) ? ((1ULL << startBit) - 1ULL) : 0ULL;
    composite &= mask;

    if (composite != 0) {
      return (startWord << 6) + (63 - halo::bits::CountLeadingZeros(composite));
    }

    for (int32_t w = startWord - 1; w >= 0; --w) {
      composite = GetCompositeWord(y, w);
      if (composite != 0) {
        return (w << 6) + (63 - halo::bits::CountLeadingZeros(composite));
      }
    }
    return 0;
  }

  [[nodiscard]] inline int32_t Width() const noexcept { return m_w; }
  [[nodiscard]] inline int32_t Height() const noexcept { return m_h; }
};

using LayeredHazardMatrix = LayeredHazardMatrixT<0, 0>;

} // namespace halo::swar