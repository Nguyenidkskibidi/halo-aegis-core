#pragma once

#include "../core/halo_memory.h"
#include "../utils/halo_types.h"
#include <cassert>
#include <cstdint>
#include <cstring>

namespace halo {

// True JPS+ Engine with Dual-Line Lookahead Prefetching and NTTP Geometry
template <int32_t W = 0, int32_t H = 0>
class alignas(64) JpsPlusEngineT {
private:
  int16_t *m_jumpTable = nullptr;
  Grid *m_grid = nullptr;
  int32_t m_w = W;
  int32_t m_h = H;

  static constexpr bool HAS_STATIC_DIM = (W > 0 && H > 0);
  static constexpr bool IS_POW2 = HAS_STATIC_DIM && IsPowerOfTwo(W);
  static constexpr int32_t SHIFT_W = IS_POW2 ? Log2Constexpr(W) : 0;

  [[nodiscard]] HALO_INLINE int32_t ToIndex(int32_t x, int32_t y) const noexcept {
    if constexpr (IS_POW2) {
      return (y << SHIFT_W) | x;
    } else {
      return y * m_w + x;
    }
  }

  [[nodiscard]] HALO_INLINE bool IsWalkable(int32_t x, int32_t y) const noexcept {
    if (static_cast<uint32_t>(x) >= static_cast<uint32_t>(m_w) || static_cast<uint32_t>(y) >= static_cast<uint32_t>(m_h)) return false;
    return m_grid->IsWalkable(ToIndex(x, y));
  }

  [[nodiscard]] HALO_INLINE bool HasForcedNeighborEast(int32_t x, int32_t y) const noexcept {
    if (!IsWalkable(x, y - 1) && IsWalkable(x + 1, y - 1)) return true;
    if (!IsWalkable(x, y + 1) && IsWalkable(x + 1, y + 1)) return true;
    return false;
  }

  [[nodiscard]] HALO_INLINE bool HasForcedNeighborWest(int32_t x, int32_t y) const noexcept {
    if (!IsWalkable(x, y - 1) && IsWalkable(x - 1, y - 1)) return true;
    if (!IsWalkable(x, y + 1) && IsWalkable(x - 1, y + 1)) return true;
    return false;
  }

  [[nodiscard]] HALO_INLINE bool HasForcedNeighborNorth(int32_t x, int32_t y) const noexcept {
    if (!IsWalkable(x - 1, y) && IsWalkable(x - 1, y - 1)) return true;
    if (!IsWalkable(x + 1, y) && IsWalkable(x + 1, y - 1)) return true;
    return false;
  }

  [[nodiscard]] HALO_INLINE bool HasForcedNeighborSouth(int32_t x, int32_t y) const noexcept {
    if (!IsWalkable(x - 1, y) && IsWalkable(x - 1, y + 1)) return true;
    if (!IsWalkable(x + 1, y) && IsWalkable(x + 1, y + 1)) return true;
    return false;
  }

  [[nodiscard]] HALO_INLINE bool HasForcedNeighborNE(int32_t x, int32_t y) const noexcept {
    if (!IsWalkable(x - 1, y) && IsWalkable(x - 1, y - 1)) return true;
    if (!IsWalkable(x, y + 1) && IsWalkable(x + 1, y + 1)) return true;
    return false;
  }

  [[nodiscard]] HALO_INLINE bool HasForcedNeighborNW(int32_t x, int32_t y) const noexcept {
    if (!IsWalkable(x + 1, y) && IsWalkable(x + 1, y - 1)) return true;
    if (!IsWalkable(x, y + 1) && IsWalkable(x - 1, y + 1)) return true;
    return false;
  }

  [[nodiscard]] HALO_INLINE bool HasForcedNeighborSW(int32_t x, int32_t y) const noexcept {
    if (!IsWalkable(x + 1, y) && IsWalkable(x + 1, y + 1)) return true;
    if (!IsWalkable(x, y - 1) && IsWalkable(x - 1, y - 1)) return true;
    return false;
  }

  [[nodiscard]] HALO_INLINE bool HasForcedNeighborSE(int32_t x, int32_t y) const noexcept {
    if (!IsWalkable(x - 1, y) && IsWalkable(x - 1, y + 1)) return true;
    if (!IsWalkable(x, y - 1) && IsWalkable(x + 1, y - 1)) return true;
    return false;
  }

  HALO_INLINE void SetDist(int32_t idx, int32_t dir, int16_t dist) noexcept { m_jumpTable[(idx << 3) | dir] = dist; }

private:
  [[gnu::cold]] [[gnu::noinline]] void SweepDirection(int32_t dir, int32_t dx, int32_t dy, int32_t ortho1 = -1,
                                                      int32_t ortho2 = -1) noexcept {
    int32_t startX = (dx > 0) ? m_w - 1 : 0;
    int32_t endX = (dx > 0) ? -1 : m_w;
    int32_t stepX = (dx > 0) ? -1 : 1;
    int32_t startY = (dy > 0) ? m_h - 1 : 0;
    int32_t endY = (dy > 0) ? -1 : m_h;
    int32_t stepY = (dy > 0) ? -1 : 1;
    bool diagonal = (ortho1 >= 0);

    for (int32_t y = startY; y != endY; y += stepY) {
      for (int32_t x = startX; x != endX; x += stepX) {
        if (!IsWalkable(x, y)) continue;
        int32_t idx = ToIndex(x, y);
        int32_t nx = x + dx;
        int32_t ny = y + dy;
        if (nx < 0 || nx >= m_w || ny < 0 || ny >= m_h || !IsWalkable(nx, ny) || (diagonal && (!IsWalkable(nx, y) || !IsWalkable(x, ny)))) {
          SetDist(idx, dir, 0);
        } else {
          int32_t nIdx = ToIndex(nx, ny);
          bool forced = (dir == Direction::EAST)        ? HasForcedNeighborEast(nx, ny)
                        : (dir == Direction::WEST)      ? HasForcedNeighborWest(nx, ny)
                        : (dir == Direction::NORTH)     ? HasForcedNeighborNorth(nx, ny)
                        : (dir == Direction::SOUTH)     ? HasForcedNeighborSouth(nx, ny)
                        : (dir == Direction::NORTHEAST) ? HasForcedNeighborNE(nx, ny)
                        : (dir == Direction::NORTHWEST) ? HasForcedNeighborNW(nx, ny)
                        : (dir == Direction::SOUTHWEST) ? HasForcedNeighborSW(nx, ny)
                                                        : HasForcedNeighborSE(nx, ny);
          if (forced || (diagonal && (GetJumpDistance(nIdx, ortho1) > 0 || GetJumpDistance(nIdx, ortho2) > 0))) {
            SetDist(idx, dir, 1);
          } else {
            int16_t nextDist = GetJumpDistance(nIdx, dir);
            SetDist(idx, dir, (nextDist > 0) ? (nextDist + 1) : (nextDist - 1));
          }
        }
      }
    }
  }

public:
  JpsPlusEngineT() noexcept = default;

  [[gnu::cold]] [[gnu::noinline]] void Precompute(Grid *grid, memory::ArenaAllocator &arena) noexcept {
    assert(grid != nullptr && "JpsPlusEngine: Grid must not be null");
    m_grid = grid;
    m_w = grid->Width();
    m_h = grid->Height();
    const int32_t size = grid->Size();

    m_jumpTable = arena.AllocateArray<int16_t, 64>(static_cast<size_t>(size) * 8);
    assert(m_jumpTable != nullptr && "JpsPlusEngine: Allocation failed");
    std::memset(m_jumpTable, 0, static_cast<size_t>(size) * 8 * sizeof(int16_t));

    // PASS 1: Orthogonal sweeps
    SweepDirection(Direction::EAST, 1, 0);
    SweepDirection(Direction::WEST, -1, 0);
    SweepDirection(Direction::NORTH, 0, -1);
    SweepDirection(Direction::SOUTH, 0, 1);

    // PASS 2: Diagonal Composite Sweeps (NE, NW, SW, SE)
    SweepDirection(Direction::NORTHEAST, 1, -1, Direction::EAST, Direction::NORTH);
    SweepDirection(Direction::NORTHWEST, -1, -1, Direction::WEST, Direction::NORTH);
    SweepDirection(Direction::SOUTHWEST, -1, 1, Direction::WEST, Direction::SOUTH);
    SweepDirection(Direction::SOUTHEAST, 1, 1, Direction::EAST, Direction::SOUTH);
  }

  [[nodiscard]] HALO_INLINE int16_t GetJumpDistance(int32_t nodeIdx, int32_t direction) const noexcept {
    assert(m_jumpTable != nullptr && "JpsPlusEngine not initialized");
    return m_jumpTable[(nodeIdx << 3) | direction];
  }
};

using JpsPlusEngine = JpsPlusEngineT<0, 0>;

}  // namespace halo