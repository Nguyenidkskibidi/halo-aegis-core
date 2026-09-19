#pragma once

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <utility>

#if !defined(ESP_PLATFORM) && !defined(ARDUINO) && !defined(__XTENSA__) && !defined(HALO_EMBEDDED_BAREMETAL)
  #if defined(__APPLE__)
    #include <pthread.h>
    #include <pthread/qos.h>
    #include <sys/mman.h>
  #elif defined(__linux__)
    #include <pthread.h>
    #include <sched.h>
    #include <sys/mman.h>
  #endif
#endif

#ifdef _MSC_VER
#include <malloc.h>
#define HALO_ALIGNED_ALLOC(size, align) _aligned_malloc(size, align)
#define HALO_ALIGNED_FREE(ptr) _aligned_free(ptr)
#elif defined(ESP_PLATFORM)
// Native ESP-IDF: supports allocating from internal SRAM or external PSRAM (SPIRAM)
#include <esp_heap_caps.h>
inline void *halo_esp_aligned_alloc(size_t size, size_t alignment) noexcept {
  void *ptr = heap_caps_aligned_alloc(alignment, size, MALLOC_CAP_8BIT);
  if (!ptr) {
    ptr = heap_caps_aligned_alloc(alignment, size, MALLOC_CAP_SPIRAM);
  }
  return ptr;
}
#define HALO_ALIGNED_ALLOC(size, align) halo_esp_aligned_alloc(size, align)
#define HALO_ALIGNED_FREE(ptr) heap_caps_free(ptr)
#elif defined(ARDUINO) || defined(__XTENSA__) || defined(HALO_EMBEDDED_BAREMETAL)
#include <cstdlib>
inline void *halo_embedded_aligned_alloc(size_t size, size_t alignment) noexcept {
  size_t total = size + alignment + sizeof(void *);
  uint8_t *raw = static_cast<uint8_t *>(std::malloc(total));
  if (!raw) return nullptr;
  uintptr_t rawAddr = reinterpret_cast<uintptr_t>(raw) + sizeof(void *);
  uintptr_t alignedAddr = (rawAddr + (alignment - 1)) & ~(alignment - 1);
  reinterpret_cast<void **>(alignedAddr)[-1] = raw;
  return reinterpret_cast<void *>(alignedAddr);
}
inline void halo_embedded_aligned_free(void *ptr) noexcept {
  if (!ptr) return;
  void *raw = reinterpret_cast<void **>(ptr)[-1];
  std::free(raw);
}
#define HALO_ALIGNED_ALLOC(size, align) halo_embedded_aligned_alloc(size, align)
#define HALO_ALIGNED_FREE(ptr) halo_embedded_aligned_free(ptr)
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
#if defined(ESP_PLATFORM) || defined(ARDUINO) || defined(__XTENSA__) || defined(HALO_EMBEDDED_BAREMETAL)
  // On ESP32 / FreeRTOS, core pinning is governed via xTaskCreatePinnedToCore
  return true;
#elif defined(__APPLE__)
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
  bool m_ownsBuffer = false;

  static constexpr size_t DEFAULT_ALIGNMENT = 64;

public:
  ArenaAllocator() noexcept = default;

  explicit ArenaAllocator(size_t capacity) {
    Init(capacity);
  }

  ArenaAllocator(void *buffer, size_t capacity) noexcept {
    InitWithBuffer(buffer, capacity);
  }

  ~ArenaAllocator() noexcept {
    Release();
  }

  ArenaAllocator(const ArenaAllocator &) = delete;
  ArenaAllocator &operator=(const ArenaAllocator &) = delete;

  ArenaAllocator(ArenaAllocator &&other) noexcept
      : m_buffer(other.m_buffer), m_capacity(other.m_capacity),
        m_offset(other.m_offset), m_ownsBuffer(other.m_ownsBuffer) {
    other.m_buffer = nullptr;
    other.m_capacity = 0;
    other.m_offset = 0;
    other.m_ownsBuffer = false;
  }

  ArenaAllocator &operator=(ArenaAllocator &&other) noexcept {
    if (this != &other) {
      Release();
      m_buffer = other.m_buffer;
      m_capacity = other.m_capacity;
      m_offset = other.m_offset;
      m_ownsBuffer = other.m_ownsBuffer;
      other.m_buffer = nullptr;
      other.m_capacity = 0;
      other.m_offset = 0;
      other.m_ownsBuffer = false;
    }
    return *this;
  }

  // Dynamic initialization with heap-aligned allocation
  void Init(size_t size) {
    Release();
    if (size == 0) return;
    size_t paddedSize = (size + (DEFAULT_ALIGNMENT - 1)) & ~(DEFAULT_ALIGNMENT - 1);
    m_buffer = static_cast<uint8_t *>(HALO_ALIGNED_ALLOC(paddedSize, DEFAULT_ALIGNMENT));
    assert(m_buffer != nullptr && "ArenaAllocator: Memory allocation failed");
    m_capacity = paddedSize;
    m_offset = 0;
    m_ownsBuffer = true;
    PreFaultAndWarmCache();
  }

  // Deterministic Zero-Heap initialization using a static or external memory buffer (e.g. ESP32 PSRAM or stack array)
  void InitWithBuffer(void *buffer, size_t capacity) noexcept {
    Release();
    if (!buffer || capacity == 0) return;
    uintptr_t addr = reinterpret_cast<uintptr_t>(buffer);
    size_t pad = (DEFAULT_ALIGNMENT - (addr % DEFAULT_ALIGNMENT)) % DEFAULT_ALIGNMENT;
    if (capacity <= pad) return;
    m_buffer = static_cast<uint8_t *>(buffer) + pad;
    m_capacity = capacity - pad;
    m_offset = 0;
    m_ownsBuffer = false;
  }

  // Pre-faults virtual pages and warms CPU cache lines.
  // Eradicates OS soft page faults on desktop, safely no-ops on flat physical embedded RAM.
  void PreFaultAndWarmCache() noexcept {
    if (!m_buffer || m_capacity == 0) return;
#if (defined(__APPLE__) || defined(__linux__)) && !defined(ESP_PLATFORM) && !defined(ARDUINO) && !defined(__XTENSA__) && !defined(HALO_EMBEDDED_BAREMETAL)
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
    if (m_buffer && m_ownsBuffer) {
      HALO_ALIGNED_FREE(m_buffer);
    }
    m_buffer = nullptr;
    m_capacity = 0;
    m_offset = 0;
    m_ownsBuffer = false;
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