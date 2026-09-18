#pragma once

#include "../core/halo_memory.h"
#include "halo_types.h"
#include <cassert>
#include <cstdint>
#include <cstring>

namespace halo {

// High-Performance Cache-Conscious 4-ary Min Heap
// Microarchitectural Optimizations:
// 1. Branchless 4-way comparison tournament network (csel on ARM64 / cmov on x86)
// 2. Hardware cache line lookahead prefetching (__builtin_prefetch)
// 3. Contiguous 4-child layout fitting within a single 64-byte cache line
class alignas(64) FourAryMinHeap {
private:
  const PathNode *m_pool = nullptr;
  int32_t m_cap = 0;
  int32_t m_size = 0;
  int32_t *m_heap = nullptr;
  int32_t *m_pos = nullptr;

  [[nodiscard]] HALO_INLINE bool Less(int32_t a, int32_t b) const noexcept {
    const PathNode &na = m_pool[a];
    const PathNode &nb = m_pool[b];
    return (na.f == nb.f) ? (na.g > nb.g) : (na.f < nb.f);
  }

public:
  FourAryMinHeap() noexcept = default;

  void Init(int32_t cap, const PathNode *pool, memory::ArenaAllocator &arena) noexcept {
    m_pool = pool;
    m_cap = cap;
    m_size = 0;
    // Pad allocation so SiftDown lookaheads and 4-wide reads never fault
    m_heap = arena.AllocateArray<int32_t, 64>(static_cast<size_t>(cap) + 8);
    m_pos = arena.AllocateArray<int32_t, 64>(static_cast<size_t>(cap) + 8);
    if (m_pos) {
      std::memset(m_pos, 0xFF, (static_cast<size_t>(cap) + 8) * sizeof(int32_t));
    }
  }

  inline void Clear() noexcept {
    m_size = 0;
  }

  [[nodiscard]] inline bool Empty() const noexcept {
    return m_size == 0;
  }

  [[nodiscard]] inline int32_t Size() const noexcept {
    return m_size;
  }

  inline void Push(int32_t idx) noexcept {
    assert(m_size < m_cap && "FourAryMinHeap overflow");
    SiftUp(m_size++, idx);
  }

  [[nodiscard]] inline int32_t Pop() noexcept {
    assert(m_size > 0 && "FourAryMinHeap underflow");
    int32_t top = m_heap[0];
    m_pos[top] = -1;
    --m_size;
    if (HALO_LIKELY(m_size > 0)) {
      SiftDown(0, m_heap[m_size]);
    }
    return top;
  }

  inline void DecreaseKey(int32_t idx) noexcept {
    int32_t pos = m_pos[idx];
    if (HALO_LIKELY(pos >= 0)) {
      SiftUp(pos, idx);
    }
  }

  [[nodiscard]] inline bool Contains(int32_t idx) const noexcept {
    return m_pos[idx] >= 0;
  }

private:
  [[gnu::noinline]] void SiftUp(int32_t pos, int32_t idx) noexcept {
    while (pos > 0) {
      int32_t parentPos = (pos - 1) >> 2;
      int32_t pIdx = m_heap[parentPos];

      if (Less(idx, pIdx)) {
        m_heap[pos] = pIdx;
        m_pos[pIdx] = pos;
        pos = parentPos;
      } else {
        break;
      }
    }
    m_heap[pos] = idx;
    m_pos[idx] = pos;
  }

  [[gnu::noinline]] void SiftDown(int32_t pos, int32_t idx) noexcept {
    while (true) {
      int32_t c0 = (pos << 2) + 1;
      if (c0 >= m_size) break;

      // Temporal prefetch: drag the 4-child contiguous cache line directly into L1D
      HALO_PREFETCH(&m_heap[c0]);

      int32_t rem = m_size - c0;
      int32_t bestChild;
      int32_t bestIdx;

      if (HALO_LIKELY(rem >= 4)) {
        int32_t idx0 = m_heap[c0];
        int32_t idx1 = m_heap[c0 + 1];
        int32_t idx2 = m_heap[c0 + 2];
        int32_t idx3 = m_heap[c0 + 3];

        bool b01 = Less(idx1, idx0);
        int32_t win01 = b01 ? (c0 + 1) : c0;
        int32_t best01 = b01 ? idx1 : idx0;

        bool b23 = Less(idx3, idx2);
        int32_t win23 = b23 ? (c0 + 3) : (c0 + 2);
        int32_t best23 = b23 ? idx3 : idx2;

        bool bFinal = Less(best23, best01);
        bestChild = bFinal ? win23 : win01;
        bestIdx = bFinal ? best23 : best01;
      } else {
        bestChild = c0;
        bestIdx = m_heap[c0];
        for (int32_t k = 1; k < rem; ++k) {
          int32_t candIdx = m_heap[c0 + k];
          if (Less(candIdx, bestIdx)) {
            bestChild = c0 + k;
            bestIdx = candIdx;
          }
        }
      }

      if (Less(bestIdx, idx)) {
        m_heap[pos] = bestIdx;
        m_pos[bestIdx] = pos;
        pos = bestChild;

        // Prefetch next generation before continuing
        HALO_PREFETCH(&m_heap[(pos << 2) + 1]);
      } else {
        break;
      }
    }
    m_heap[pos] = idx;
    m_pos[idx] = pos;
  }
};

using BinaryMinHeap = FourAryMinHeap;
using MinHeap4Way = FourAryMinHeap;

} // namespace halo