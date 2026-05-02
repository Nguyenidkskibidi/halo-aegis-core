#pragma once
#include <cassert>
#include <cstdint>
#include <cstdlib>

#ifdef _MSC_VER
#include <malloc.h>
#define HALO_ALLOC(s, a) _aligned_malloc(s, a)
#define HALO_FREE(p) _aligned_free(p)
#else
inline void *halo_posix_align(size_t s, size_t a) {
  void *p = nullptr;
  posix_memalign(&p, a, s);
  return p;
}
#define HALO_ALLOC(s, a) halo_posix_align(s, a)
#define HALO_FREE(p) free(p)
#endif

namespace halo::memory {
class ArenaAllocator {
  void *m_buf = nullptr;
  size_t m_cap = 0, m_off = 0;

public:
  ~ArenaAllocator() {
    if (m_buf)
      HALO_FREE(m_buf);
  }
  void Init(size_t size) {
    m_buf = HALO_ALLOC(size, 64);
    m_cap = size;
    m_off = 0;
  }
  template <typename T> T *AllocateArray(size_t count) {
    size_t b = count * sizeof(T), a = alignof(T);
    size_t p = (a - (reinterpret_cast<uintptr_t>(m_buf) + m_off) % a) % a;
    void *ptr = static_cast<uint8_t *>(m_buf) + m_off + p;
    m_off += p + b;
    return static_cast<T *>(ptr);
  }
  void Reset() { m_off = 0; }
};
} // namespace halo::memory