#pragma once

#include "../core/halo_memory.h"
#include "../utils/halo_types.h"
#include <cstring>
#include <iostream>

namespace halo {

class JpsPlusEngine {
  int32_t *m_jumpTable = nullptr;
  Grid *m_grid = nullptr;

public:
  void Precompute(Grid *grid, memory::ArenaAllocator &arena) noexcept {
    m_grid = grid;
    int32_t size = grid->Size();

    m_jumpTable = arena.AllocateArray<int32_t>(size * 8);
    std::memset(m_jumpTable, 0, size * 8 * sizeof(int32_t));

    std::cout << "⚡ [JPS+ ENGINE]: Studying...";

    int32_t w = grid->Width();
    int32_t h = grid->Size() / w;

    for (int y = 0; y < h; ++y) {
      for (int x = 0; x < w; ++x) {
        int32_t idx = grid->ToIndex(x, y);
        if (!grid->IsWalkable(idx))
          continue;

        if (x > 0 && grid->IsWalkable(idx - 1))
          m_jumpTable[idx * 8 + 4] = m_jumpTable[(idx - 1) * 8 + 4] + 1;

        if (y > 0 && grid->IsWalkable(idx - w))
          m_jumpTable[idx * 8 + 2] = m_jumpTable[(idx - w) * 8 + 2] + 1;

        if (x > 0 && y > 0 && grid->IsWalkable(idx - w - 1) &&
            grid->IsWalkable(idx - 1) && grid->IsWalkable(idx - w))
          m_jumpTable[idx * 8 + 3] = m_jumpTable[(idx - w - 1) * 8 + 3] + 1;

        if (x < w - 1 && y > 0 && grid->IsWalkable(idx - w + 1) &&
            grid->IsWalkable(idx + 1) && grid->IsWalkable(idx - w))
          m_jumpTable[idx * 8 + 1] = m_jumpTable[(idx - w + 1) * 8 + 1] + 1;
      }
    }

    for (int y = h - 1; y >= 0; --y) {
      for (int x = w - 1; x >= 0; --x) {
        int32_t idx = grid->ToIndex(x, y);
        if (!grid->IsWalkable(idx))
          continue;

        if (x < w - 1 && grid->IsWalkable(idx + 1))
          m_jumpTable[idx * 8 + 0] = m_jumpTable[(idx + 1) * 8 + 0] + 1;

        if (y < h - 1 && grid->IsWalkable(idx + w))
          m_jumpTable[idx * 8 + 6] = m_jumpTable[(idx + w) * 8 + 6] + 1;

        if (x < w - 1 && y < h - 1 && grid->IsWalkable(idx + w + 1) &&
            grid->IsWalkable(idx + 1) && grid->IsWalkable(idx + w))
          m_jumpTable[idx * 8 + 7] = m_jumpTable[(idx + w + 1) * 8 + 7] + 1;

        if (x > 0 && y < h - 1 && grid->IsWalkable(idx + w - 1) &&
            grid->IsWalkable(idx - 1) && grid->IsWalkable(idx + w))
          m_jumpTable[idx * 8 + 5] = m_jumpTable[(idx + w - 1) * 8 + 5] + 1;
      }
    }
  }

  [[nodiscard]] inline int32_t
  GetJumpDistance(int32_t nodeIdx, int32_t direction) const noexcept {
    return m_jumpTable[nodeIdx * 8 + direction];
  }
};

} // namespace halo