/**
 * ============================================================================
 *  H.A.L.O.* — Pure Integer Binary Min-Heap (Arena Integrated)
 * ============================================================================
 *  Vị trí mới: include/halo/utils/halo_heap.h
 *  Sửa lỗi: Link lại include để tìm thấy file ở folder core và utils
 * ============================================================================
 */
#pragma once

// Sửa đường dẫn để tìm thấy types (cùng folder utils) và memory (ở folder core)
#include "halo_types.h"            
#include "../core/halo_memory.h"  

namespace halo {
class BinaryMinHeap {
    const PathNode* m_pool = nullptr; 
    int32_t m_cap = 0, m_size = 0;
    int32_t* m_heap = nullptr; 
    int32_t* m_pos = nullptr;
public:
    // Nhận Arena vào để bú RAM trực tiếp từ hệ thống Memory Core
    void Init(int32_t cap, const PathNode* pool, memory::ArenaAllocator& arena) noexcept {
        m_pool = pool; m_cap = cap; m_size = 0;
        m_heap = arena.AllocateArray<int32_t>(cap); 
        m_pos = arena.AllocateArray<int32_t>(cap);
        std::memset(m_pos, 0xFF, cap * sizeof(int32_t));
    }
    
    inline void Clear() noexcept { m_size = 0; }
    inline bool Empty() const noexcept { return m_size == 0; }
    inline void Push(int32_t i) noexcept { SiftUp(m_size++, i); }
    inline int32_t Pop() noexcept { 
        int32_t t = m_heap[0]; m_pos[t] = -1; 
        if (--m_size > 0) SiftDown(0, m_heap[m_size]); 
        return t; 
    }
    inline void DecreaseKey(int32_t i) noexcept { SiftUp(m_pos[i], i); }

private:
    inline void SiftUp(int32_t pos, int32_t idx) noexcept {
        while (pos > 0) {
            int32_t p = (pos - 1) >> 1, pIdx = m_heap[p];
            // So sánh f-cost, nếu bằng nhau thì ưu tiên g-cost cao (phá vỡ rào cản tie-break)
            if ((m_pool[idx].f == m_pool[pIdx].f) ? (m_pool[idx].g > m_pool[pIdx].g) : (m_pool[idx].f < m_pool[pIdx].f)) {
                m_heap[pos] = pIdx; m_pos[pIdx] = pos; pos = p;
            } else break;
        }
        m_heap[pos] = idx; m_pos[idx] = pos;
    }
    
    inline void SiftDown(int32_t pos, int32_t idx) noexcept {
        int32_t half = m_size >> 1;
        while (pos < half) {
            int32_t c = (pos << 1) + 1, cIdx = m_heap[c];
            if (c + 1 < m_size && ((m_pool[m_heap[c+1]].f == m_pool[cIdx].f) ? (m_pool[m_heap[c+1]].g > m_pool[cIdx].g) : (m_pool[m_heap[c+1]].f < m_pool[cIdx].f)))
                cIdx = m_heap[++c];
            if ((m_pool[idx].f == m_pool[cIdx].f) ? (m_pool[idx].g > m_pool[cIdx].g) : (m_pool[idx].f < m_pool[cIdx].f)) break;
            m_heap[pos] = cIdx; m_pos[cIdx] = pos; pos = c;
        }
        m_heap[pos] = idx; m_pos[idx] = pos;
    }
};
} // namespace halo