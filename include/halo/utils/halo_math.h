#pragma once

#include "halo_types.h"
#include <cmath>
#include <cstdint>
#include <cstdlib>

namespace halo::math {

[[nodiscard]] HALO_INLINE int32_t
OctileDistanceFP(Vec2i current, Vec2i target) noexcept {
  int32_t dx = std::abs(current.x - target.x);
  int32_t dy = std::abs(current.y - target.y);
  int32_t mn = (dx < dy) ? dx : dy;
  int32_t mx = (dx > dy) ? dx : dy;
  return (mx << 10) + Config::SQRT2_MINUS_1_FP * mn;
}

[[nodiscard]] HALO_INLINE int32_t
OctileDistanceFP_TieBreak(Vec2i current, Vec2i target, Vec2i start) noexcept {
  int32_t dx = std::abs(current.x - target.x);
  int32_t dy = std::abs(current.y - target.y);
  int32_t mn = (dx < dy) ? dx : dy;
  int32_t mx = (dx > dy) ? dx : dy;
  int32_t h = (mx << 10) + Config::SQRT2_MINUS_1_FP * mn;

  int64_t dx1 = current.x - target.x;
  int64_t dy1 = current.y - target.y;
  int64_t dx2 = start.x - target.x;
  int64_t dy2 = start.y - target.y;
  int64_t cross = std::abs(dx1 * dy2 - dx2 * dy1);

  return h + static_cast<int32_t>(cross >> 6);
}

[[nodiscard]] HALO_INLINE int32_t
GetDynamicWeightFP(int32_t startDist, int32_t h) noexcept {
  if (startDist <= 0) return 1024;
  return 1024 + static_cast<int32_t>(static_cast<int64_t>(Config::WEIGHT_MUL) * 1024 * h / startDist);
}

[[nodiscard]] inline bool HasLineOfSight(const Grid &g, Vec2i p0, Vec2i p1) noexcept {
  int32_t dx = std::abs(p1.x - p0.x);
  int32_t dy = -std::abs(p1.y - p0.y);
  int32_t sx = (p0.x < p1.x) ? 1 : -1;
  int32_t sy = (p0.y < p1.y) ? 1 : -1;
  int32_t err = dx + dy;

  while (true) {
    if (!g.IsWalkable(g.ToIndex(p0))) return false;
    if (p0 == p1) break;

    int32_t e2 = 2 * err;
    if (e2 >= dy) {
      err += dy;
      p0.x += sx;
    }
    if (e2 <= dx) {
      err += dx;
      p0.y += sy;
    }
  }
  return true;
}

} // namespace halo::math