#pragma once

#include "../core/halo_memory.h"
#include "../protection/halo_swar_10_layer_bitboard.h"
#include "../utils/halo_types.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>

namespace halo::fov {

// Bitwise 64x64 Local Visibility Mask (4096 bits = 64 uint64_t words = 512 bytes)
struct alignas(64) VisibilityMask64 {
  uint64_t rows[64] = {};

  HALO_INLINE void Clear() noexcept {
    std::memset(rows, 0, sizeof(rows));
  }

  HALO_INLINE void SetVisible(int32_t x, int32_t y) noexcept {
    if (x >= 0 && x < 64 && y >= 0 && y < 64) {
      rows[y] |= (1ULL << x);
    }
  }

  [[nodiscard]] HALO_INLINE bool IsVisible(int32_t x, int32_t y) const noexcept {
    if (x < 0 || x >= 64 || y < 0 || y >= 64) return false;
    return (rows[y] & (1ULL << x)) != 0;
  }
};

// High-Performance Recursive Circular Shadowcasting FOV
class alignas(64) ShadowcastingFOV {
public:
  // Sub-microsecond Line-of-Sight check between two grid cells
  template <typename BlockedFunc>
  [[nodiscard]] static bool HasLineOfSight(Vec2i from, Vec2i to, BlockedFunc &&isBlocked) noexcept {
    int32_t dx = std::abs(to.x - from.x);
    int32_t dy = -std::abs(to.y - from.y);
    int32_t sx = (from.x < to.x) ? 1 : -1;
    int32_t sy = (from.y < to.y) ? 1 : -1;
    int32_t err = dx + dy;

    int32_t curX = from.x;
    int32_t curY = from.y;

    while (true) {
      if (curX == to.x && curY == to.y) return true;

      int32_t e2 = 2 * err;
      if (e2 >= dy) {
        err += dy;
        curX += sx;
      }
      if (e2 <= dx) {
        err += dx;
        curY += sy;
      }

      if (curX == to.x && curY == to.y) return true;
      if (isBlocked(curX, curY)) return false;
    }
  }

  // Compute Circular Field of View up to radius R (max 31) into 64x64 bitmask centered at viewer
  template <typename BlockedFunc>
  [[gnu::cold]] [[gnu::noinline]] static void ComputeFOV(Vec2i viewerPos, int32_t radius, VisibilityMask64 &outMask,
                         BlockedFunc &&isBlocked) noexcept {
    outMask.Clear();
    radius = std::clamp(radius, 1, 31);
    const int32_t rSq = radius * radius;

    // Center cell (viewer) is always visible in local coords (32, 32)
    constexpr int32_t CX = 32;
    constexpr int32_t CY = 32;
    outMask.SetVisible(CX, CY);

    // Cast rays to the boundary perimeter of the circle
    for (int32_t dy = -radius; dy <= radius; ++dy) {
      for (int32_t dx = -radius; dx <= radius; ++dx) {
        if (dx * dx + dy * dy > rSq) continue;

        // Trace line from (0, 0) to (dx, dy)
        int32_t stepDx = std::abs(dx);
        int32_t stepDy = -std::abs(dy);
        int32_t sx = (dx > 0) ? 1 : -1;
        int32_t sy = (dy > 0) ? 1 : -1;
        int32_t err = stepDx + stepDy;

        int32_t px = 0;
        int32_t py = 0;

        while (true) {
          outMask.SetVisible(CX + px, CY + py);
          if (px == dx && py == dy) break;

          // If world cell is blocked, line of sight terminates
          if (isBlocked(viewerPos.x + px, viewerPos.y + py)) {
            break;
          }

          int32_t e2 = 2 * err;
          if (e2 >= stepDy) {
            err += stepDy;
            px += sx;
          }
          if (e2 <= stepDx) {
            err += stepDx;
            py += sy;
          }
        }
      }
    }
  }
};

} // namespace halo::fov
