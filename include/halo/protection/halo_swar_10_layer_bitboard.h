#pragma once

#include "../utils/halo_types.h"
#include <bit>
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

    for (int i = 0; i < m_w; ++i) {
      SetBit(Layer::STATIC_WALLS, i, 0);
      SetBit(Layer::STATIC_WALLS, i, m_h - 1);
    }
    for (int i = 0; i < m_h; ++i) {
      SetBit(Layer::STATIC_WALLS, 0, i);
      SetBit(Layer::STATIC_WALLS, m_w - 1, i);
    }
  }

  inline void SetBit(Layer layer, int32_t x, int32_t y) noexcept {
    if (x < 0 || x >= 64 || y < 0 || y >= 64)
      return;
    uint8_t l = static_cast<uint8_t>(layer);
    m_rows[l][y] |= (1ULL << x);
    m_cols[l][x] |= (1ULL << y);
  }

  inline bool IsBitSet(Layer layer, int32_t x, int32_t y) const noexcept {
    if (x < 0 || x >= 64 || y < 0 || y >= 64)
      return false;
    return (m_rows[static_cast<uint8_t>(layer)][y] & (1ULL << x)) != 0;
  }

  void ClearAllLayers() noexcept {
    std::memset(m_rows, 0, sizeof(m_rows));
    std::memset(m_cols, 0, sizeof(m_cols));
  }

  inline uint64_t GetCompositeRow(int32_t y) const noexcept {
    return m_rows[0][y] | m_rows[1][y] | m_rows[2][y] | m_rows[3][y] |
           m_rows[4][y] | m_rows[5][y] | m_rows[6][y] | m_rows[7][y] |
           m_rows[8][y] | m_rows[9][y];
  }

  inline uint64_t GetCompositeCol(int32_t x) const noexcept {
    return m_cols[0][x] | m_cols[1][x] | m_cols[2][x] | m_cols[3][x] |
           m_cols[4][x] | m_cols[5][x] | m_cols[6][x] | m_cols[7][x] |
           m_cols[8][x] | m_cols[9][x];
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