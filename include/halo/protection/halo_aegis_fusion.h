#pragma once

#include "../utils/halo_types.h"
#include "halo_swar_10_layer_bitboard.h"
#include <cmath>
#include <cstdlib>

namespace halo::aegis {

enum class ThreatType : uint8_t { BALLISTIC_PROJECTILE = 0, EMP_JAMMER_WAVE = 1, THERMAL_EXPLOSION = 2, DYNAMIC_SWARM = 3 };

struct Threat {
  ThreatType type = ThreatType::BALLISTIC_PROJECTILE;
  Vec2i origin = {0, 0};
  Vec2i velocity = {0, 0};
  int32_t coneWidth = 0;
};

class AegisFusionEngine {
private:
  swar::UltimateBitboard64 *m_aegisGrid = nullptr;

public:
  AegisFusionEngine() noexcept = default;
  explicit AegisFusionEngine(swar::UltimateBitboard64 *grid) noexcept : m_aegisGrid(grid) {}

  void BindGrid(swar::UltimateBitboard64 *grid) noexcept { m_aegisGrid = grid; }

  // Zero-allocation Bresenham ray injection for ballistic threats
  void InjectBallisticThreat(Vec2i startPos, Vec2i velocityVector, int32_t steps = 10) noexcept {
    if (HALO_UNLIKELY(!m_aegisGrid)) return;

    int32_t x0 = startPos.x;
    int32_t y0 = startPos.y;
    int32_t x1 = startPos.x + velocityVector.x * steps;
    int32_t y1 = startPos.y + velocityVector.y * steps;

    int32_t dx = std::abs(x1 - x0);
    int32_t sx = (x0 < x1) ? 1 : -1;
    int32_t dy = -std::abs(y1 - y0);
    int32_t sy = (y0 < y1) ? 1 : -1;
    int32_t err = dx + dy;

    while (true) {
      m_aegisGrid->SetBit(swar::Layer::BALLISTIC, x0, y0);
      if (x0 == x1 && y0 == y1) break;
      int32_t e2 = 2 * err;
      if (e2 >= dy) {
        err += dy;
        x0 += sx;
      }
      if (e2 <= dx) {
        err += dx;
        y0 += sy;
      }
    }
  }

  // Zero-allocation EMP jammer wave cone expansion
  void InjectEmpWaveThreat(Vec2i origin, int32_t maxRadius) noexcept {
    if (HALO_UNLIKELY(!m_aegisGrid)) return;

    for (int32_t r = 0; r < maxRadius; ++r) {
      int32_t currentY = origin.y - r;
      if (currentY < 0) break;

      int32_t spread = r >> 1;
      for (int32_t dx = -spread; dx <= spread; ++dx) {
        int32_t currentX = origin.x + dx;
        if (currentX >= 0 && currentX < m_aegisGrid->Width()) {
          m_aegisGrid->SetBit(swar::Layer::BROADCAST_TOWERS, currentX, currentY);
        }
      }
    }
  }

  // Template methods for generalized hazard matrices
  template <typename GridT>
  static void InjectBallisticThreatGeneric(GridT &grid, Vec2i startPos, Vec2i velocityVector, int32_t steps = 10) noexcept {
    int32_t x0 = startPos.x;
    int32_t y0 = startPos.y;
    int32_t x1 = startPos.x + velocityVector.x * steps;
    int32_t y1 = startPos.y + velocityVector.y * steps;

    int32_t dx = std::abs(x1 - x0);
    int32_t sx = (x0 < x1) ? 1 : -1;
    int32_t dy = -std::abs(y1 - y0);
    int32_t sy = (y0 < y1) ? 1 : -1;
    int32_t err = dx + dy;

    while (true) {
      grid.SetBit(swar::Layer::BALLISTIC, x0, y0);
      if (x0 == x1 && y0 == y1) break;
      int32_t e2 = 2 * err;
      if (e2 >= dy) {
        err += dy;
        x0 += sx;
      }
      if (e2 <= dx) {
        err += dx;
        y0 += sy;
      }
    }
  }
};

}  // namespace halo::aegis