#pragma once

#include "../utils/halo_types.h"
#include <cmath>
#include <vector>

namespace halo::postprocess {

inline std::vector<Vec2i>
RasterizePath(const std::vector<Vec2i> &waypoints) noexcept {
  std::vector<Vec2i> fullPath;
  if (waypoints.empty())
    return fullPath;

  fullPath.reserve(waypoints.size() * 10);
  fullPath.push_back(waypoints[0]);

  for (size_t i = 0; i < waypoints.size() - 1; ++i) {
    Vec2i p0 = waypoints[i];
    Vec2i p1 = waypoints[i + 1];

    int32_t dx = std::abs(p1.x - p0.x);
    int32_t sx = p0.x < p1.x ? 1 : -1;
    int32_t dy = -std::abs(p1.y - p0.y);
    int32_t sy = p0.y < p1.y ? 1 : -1;
    int32_t err = dx + dy, e2;

    while (true) {

      if (p0.x == p1.x && p0.y == p1.y)
        break;

      e2 = 2 * err;
      if (e2 >= dy) {
        err += dy;
        p0.x += sx;
      }
      if (e2 <= dx) {
        err += dx;
        p0.y += sy;
      }

      if (p0.x != p1.x || p0.y != p1.y) {
        fullPath.push_back(p0);
      }
    }
    fullPath.push_back(p1);
  }
  return fullPath;
}

} // namespace halo::postprocess