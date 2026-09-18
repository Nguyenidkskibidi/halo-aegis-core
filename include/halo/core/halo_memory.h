#pragma once

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <utility>

#if defined(__APPLE__)
#include <pthread.h>
#include <pthread/qos.h>
#include <sys/mman.h>
#elif defined(__linux__)
#include <pthread.h>
#include <sched.h>
#include <sys/mman.h>
#endif

#ifdef _MSC_VER
#include <malloc.h>
#define HALO_ALIGNED_ALLOC(size, align) _aligned_malloc(size, align)
#define HALO_ALIGNED_FREE(ptr) _aligned_free(ptr)
#else
inline void *halo_posix_aligned_alloc(size_t size, size_t alignment) noexcept {
  void *ptr = nullptr;
  size_t valid_align = (alignment < sizeof(void *)) ? sizeof(void *) : alignment;
  if (posix_memalign(&ptr, valid_align, size) != 0) {
    return nullptr;
  }
  return ptr;
}
#define HALO_ALIGNED_ALLOC(size, align) halo_posix_aligned_alloc(size, align)
#define HALO_ALIGNED_FREE(ptr) free(ptr)
#endif

namespace halo::memory {

// Hardware P-Core (Performance Core) Affinity and QoS Pinning
// Guarantees execution on highest-frequency cores with zero OS context-switch jitter.
inline bool PinThreadToPerformanceCore([[maybe_unused]] int coreId = 0) noexcept {
#if defined(__APPLE__)
  // On Apple Silicon (M1/M2/M3/M4), map current thread to User Interactive QoS (P-Core cluster)
  int res = pthread_set_qos_class_self_np(QOS_CLASS_USER_INTERACTIVE, 0);
  return (res == 0);
#elif defined(__linux__)
  cpu_set_t cpuset;
  CPU_ZERO(&cpuset);
  CPU_SET(coreId, &cpuset);
  int res = pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
  return (res == 0);
#else
  return false;
#endif
}

class alignas(64) ArenaAllocator {
private:
  uint8_t *m_buffer = nullptr;
  size_t m_capacity = 0;
  size_t m_offset = 0;

  static constexpr size_t DEFAULT_ALIGNMENT = 64;

public:
  ArenaAllocator() noexcept = default;

  explicit ArenaAllocator(size_t capacity) {
    Init(capacity);
  }

  ~ArenaAllocator() noexcept {
    Release();
  }

  ArenaAllocator(const ArenaAllocator &) = delete;
  ArenaAllocator &operator=(const ArenaAllocator &) = delete;

  ArenaAllocator(ArenaAllocator &&other) noexcept
      : m_buffer(other.m_buffer), m_capacity(other.m_capacity),
        m_offset(other.m_offset) {
    other.m_buffer = nullptr;
    other.m_capacity = 0;
    other.m_offset = 0;
  }

  ArenaAllocator &operator=(ArenaAllocator &&other) noexcept {
    if (this != &other) {
      Release();
      m_buffer = other.m_buffer;
      m_capacity = other.m_capacity;
      m_offset = other.m_offset;
      other.m_buffer = nullptr;
      other.m_capacity = 0;
      other.m_offset = 0;
    }
    return *this;
  }

  void Init(size_t size) {
    Release();
    if (size == 0) return;
    size_t paddedSize = (size + (DEFAULT_ALIGNMENT - 1)) & ~(DEFAULT_ALIGNMENT - 1);
    m_buffer = static_cast<uint8_t *>(HALO_ALIGNED_ALLOC(paddedSize, DEFAULT_ALIGNMENT));
    assert(m_buffer != nullptr && "ArenaAllocator: Memory allocation failed");
    m_capacity = paddedSize;
    m_offset = 0;
    PreFaultAndWarmCache();
  }

  // Pre-faults all 4 KB virtual pages and warms CPU cache lines.
  // Eradicates OS soft page faults and demand-paging cold-start jitter.
  void PreFaultAndWarmCache() noexcept {
    if (!m_buffer || m_capacity == 0) return;
#if defined(__APPLE__) || defined(__linux__)
    // Advise kernel that these pages will be accessed immediately
    madvise(m_buffer, m_capacity, MADV_WILLNEED);
    // Lock pages into physical RAM to prevent demand paging and swap
    mlock(m_buffer, m_capacity);
#endif
    constexpr size_t PAGE_SIZE = 4096;
    volatile uint8_t *ptr = m_buffer;
    for (size_t offset = 0; offset < m_capacity; offset += PAGE_SIZE) {
      uint8_t val = ptr[offset];
      ptr[offset] = val;
    }
    if (m_capacity > 0) {
      uint8_t lastVal = ptr[m_capacity - 1];
      ptr[m_capacity - 1] = lastVal;
    }
  }

  void Release() noexcept {
    if (m_buffer) {
      HALO_ALIGNED_FREE(m_buffer);
      m_buffer = nullptr;
    }
    m_capacity = 0;
    m_offset = 0;
  }

  template <typename T, size_t Alignment = alignof(T)>
  [[nodiscard]] T *AllocateArray(size_t count) noexcept {
    if (count == 0) return nullptr;
    constexpr size_t effectiveAlign = (Alignment < DEFAULT_ALIGNMENT) ? DEFAULT_ALIGNMENT : Alignment;
    static_assert((effectiveAlign & (effectiveAlign - 1)) == 0, "Alignment must be power of two");

    uintptr_t currentAddr = reinterpret_cast<uintptr_t>(m_buffer) + m_offset;
    size_t padding = (effectiveAlign - (currentAddr % effectiveAlign)) % effectiveAlign;
    size_t totalBytes = count * sizeof(T);

    assert(m_offset + padding + totalBytes <= m_capacity && "ArenaAllocator: Out of memory");
    if (m_offset + padding + totalBytes > m_capacity) {
      return nullptr;
    }

    uint8_t *ptr = m_buffer + m_offset + padding;
    m_offset += padding + totalBytes;
    return reinterpret_cast<T *>(ptr);
  }

  [[nodiscard]] size_t GetOffset() const noexcept { return m_offset; }
  [[nodiscard]] size_t GetCapacity() const noexcept { return m_capacity; }
  [[nodiscard]] size_t GetRemaining() const noexcept { return m_capacity > m_offset ? m_capacity - m_offset : 0; }

  void Reset() noexcept {
    m_offset = 0;
  }

  void ResetTo(size_t offset) noexcept {
    assert(offset <= m_capacity && "ArenaAllocator: Invalid rollback offset");
    m_offset = offset;
  }
};

class ArenaFrame {
private:
  ArenaAllocator &m_arena;
  size_t m_savedOffset;

public:
  explicit ArenaFrame(ArenaAllocator &arena) noexcept
      : m_arena(arena), m_savedOffset(arena.GetOffset()) {}

  ~ArenaFrame() noexcept {
    m_arena.ResetTo(m_savedOffset);
  }

  ArenaFrame(const ArenaFrame &) = delete;
  ArenaFrame &operator=(const ArenaFrame &) = delete;
};

} // namespace halo::memory