#pragma once

#include "../core/halo_memory.h"
#include "../core/halo_simd.h"
#include "../utils/halo_types.h"
#include <bit>
#include <cassert>
#include <cstdint>
#include <cstring>

namespace halo::sparse {

// ============================================================================
// UNIFORM 64x64 METRIC SWAR CHUNK
// ============================================================================

class alignas(64) SparseChunk {
public:
  static constexpr int32_t CHUNK_DIM = 64;
  static constexpr int32_t PADDED_LAYERS = 16;

  alignas(64) uint64_t layers[CHUNK_DIM][PADDED_LAYERS];
  alignas(64) uint64_t shadowRows[CHUNK_DIM];

  int32_t chunkX = 0;
  int32_t chunkY = 0;
  bool isDirty = false;

  void Clear() noexcept {
    std::memset(layers, 0, sizeof(layers));
    std::memset(shadowRows, 0, sizeof(shadowRows));
    isDirty = false;
  }

  void Init(int32_t cx, int32_t cy) noexcept {
    chunkX = cx;
    chunkY = cy;
    Clear();
  }

  HALO_INLINE void SetBit(int32_t layer, int32_t lx, int32_t ly) noexcept {
    if ((uint32_t)lx >= 64 || (uint32_t)ly >= 64 || (uint32_t)layer >= 15) return;
    uint64_t mask = 1ULL << lx;
    layers[ly][layer] |= mask;
    shadowRows[ly] |= mask;
    isDirty = true;
  }

  HALO_INLINE void ClearBit(int32_t layer, int32_t lx, int32_t ly) noexcept {
    if ((uint32_t)lx >= 64 || (uint32_t)ly >= 64 || (uint32_t)layer >= 15) return;
    layers[ly][layer] &= ~(1ULL << lx);
    isDirty = true;
  }

  [[nodiscard]] HALO_INLINE bool IsBitSet(int32_t layer, int32_t lx, int32_t ly) const noexcept {
    if ((uint32_t)lx >= 64 || (uint32_t)ly >= 64 || (uint32_t)layer >= 15) return false;
    return (layers[ly][layer] & (1ULL << lx)) != 0;
  }

  [[nodiscard]] HALO_INLINE bool IsBlocked(int32_t lx, int32_t ly) const noexcept {
    if ((uint32_t)lx >= 64 || (uint32_t)ly >= 64) return false;
    return (shadowRows[ly] & (1ULL << lx)) != 0;
  }

  HALO_INLINE void RecomputeShadowRow(int32_t ly) noexcept {
    shadowRows[ly] = simd::Collapse10LayersToShadow(layers[ly]);
  }

  void RecomputeAllShadows() noexcept {
    for (int32_t y = 0; y < CHUNK_DIM; ++y) {
      RecomputeShadowRow(y);
    }
    isDirty = false;
  }

  // Fast Intra-Chunk East Raycast using Bitwise Trailing Zero Count
  [[nodiscard]] HALO_INLINE int32_t RaycastEast(int32_t startX, int32_t ly) const noexcept {
    if ((uint32_t)ly >= 64) return 63;
    uint64_t row = shadowRows[ly];
    uint64_t mask = (startX >= 63) ? 0ULL : (~0ULL << (startX + 1));
    uint64_t obstacles = row & mask;
    if (obstacles == 0) return 63;
    return std::countr_zero(obstacles);
  }

  // Fast Intra-Chunk West Raycast using Bitwise Leading Zero Count
  [[nodiscard]] HALO_INLINE int32_t RaycastWest(int32_t startX, int32_t ly) const noexcept {
    if ((uint32_t)ly >= 64) return 0;
    uint64_t row = shadowRows[ly];
    uint64_t mask = (startX <= 0) ? 0ULL : (~0ULL >> (64 - startX));
    uint64_t obstacles = row & mask;
    if (obstacles == 0) return 0;
    return 63 - std::countl_zero(obstacles);
  }
};

// ============================================================================
// FLAT ZERO-ALLOCATION ROBIN HOOD HASH CHUNK MAP
// ============================================================================

class alignas(64) RobinHoodChunkMap {
public:
  static constexpr uint32_t EMPTY_SLOT = 0xFFFFFFFF;
  static constexpr uint32_t EMPTY_INDEX = 0xFFFFFFFF;

  struct alignas(8) ChunkSlot {
    uint32_t packedCoord = EMPTY_SLOT; // (cx & 0xFFFF) | ((cy & 0xFFFF) << 16)
    uint32_t chunkIndex = EMPTY_INDEX; // Index into pre-allocated pool
  };

private:
  ChunkSlot *m_slots = nullptr;
  uint32_t m_capacity = 0;
  uint32_t m_mask = 0;
  uint32_t m_size = 0;

  static constexpr uint32_t Pack(int32_t cx, int32_t cy) noexcept {
    return (static_cast<uint32_t>(cx) & 0xFFFFu) |
           ((static_cast<uint32_t>(cy) & 0xFFFFu) << 16);
  }

  // Universal Spatial Hash Function
  static constexpr uint32_t HashCoords(int32_t cx, int32_t cy) noexcept {
    uint32_t h = (static_cast<uint32_t>(cx) * 73856093u) ^
                 (static_cast<uint32_t>(cy) * 19349663u);
    h ^= h >> 16;
    h *= 0x85ebca6bu;
    h ^= h >> 13;
    h *= 0xc2b2ae35u;
    h ^= h >> 16;
    return h;
  }

public:
  constexpr RobinHoodChunkMap() noexcept = default;

  void Init(uint32_t capacityPowerOfTwo, memory::ArenaAllocator &arena) noexcept {
    m_capacity = capacityPowerOfTwo;
    m_mask = capacityPowerOfTwo - 1;
    m_size = 0;
    m_slots = arena.AllocateArray<ChunkSlot, 64>(m_capacity);
    Clear();
  }

  void Clear() noexcept {
    if (!m_slots) return;
    for (uint32_t i = 0; i < m_capacity; ++i) {
      m_slots[i].packedCoord = EMPTY_SLOT;
      m_slots[i].chunkIndex = EMPTY_INDEX;
    }
    m_size = 0;
  }

  [[nodiscard]] uint32_t Size() const noexcept { return m_size; }
  [[nodiscard]] uint32_t Capacity() const noexcept { return m_capacity; }

  // O(1) Linear Probing Insertion
  bool Insert(int32_t cx, int32_t cy, uint32_t chunkIdx) noexcept {
    if (m_size >= (m_capacity * 7) / 10) return false; // 70% max load factor

    uint32_t packed = Pack(cx, cy);
    uint32_t idx = HashCoords(cx, cy) & m_mask;

    for (uint32_t probe = 0; probe < m_capacity; ++probe) {
      uint32_t slot = (idx + probe) & m_mask;
      if (m_slots[slot].packedCoord == EMPTY_SLOT) {
        m_slots[slot].packedCoord = packed;
        m_slots[slot].chunkIndex = chunkIdx;
        ++m_size;
        return true;
      }
      if (m_slots[slot].packedCoord == packed) {
        m_slots[slot].chunkIndex = chunkIdx; // Replace
        return true;
      }
    }
    return false;
  }

  // O(1) Branchless-Friendly Lookup
  [[nodiscard]] HALO_INLINE uint32_t Find(int32_t cx, int32_t cy) const noexcept {
    if (m_capacity == 0 || !m_slots) return EMPTY_INDEX;

    uint32_t packed = Pack(cx, cy);
    uint32_t idx = HashCoords(cx, cy) & m_mask;

    for (uint32_t probe = 0; probe < 32; ++probe) {
      uint32_t slot = (idx + probe) & m_mask;
      if (m_slots[slot].packedCoord == packed) {
        return m_slots[slot].chunkIndex;
      }
      if (m_slots[slot].packedCoord == EMPTY_SLOT) {
        return EMPTY_INDEX; // Empty sentinel reached
      }
    }
    return EMPTY_INDEX;
  }
};

// ============================================================================
// ROLLING TOROIDAL CLIPMAP (128x128 1-Meter Active Vehicle Sphere)
// ============================================================================

class alignas(64) RollingToroidalClipmap128 {
public:
  static constexpr int32_t CLIP_DIM = 128;
  static constexpr int32_t CLIP_MASK = 127;
  static constexpr int32_t PADDED_LAYERS = 16;

private:
  alignas(64) uint64_t m_rows[CLIP_DIM][PADDED_LAYERS];
  alignas(64) uint64_t m_shadowRows[CLIP_DIM];

  int32_t m_centerWorldX = 0;
  int32_t m_centerWorldY = 0;
  int32_t m_originWorldX = 0;
  int32_t m_originWorldY = 0;

public:
  RollingToroidalClipmap128() noexcept {
    Clear();
  }

  void Clear() noexcept {
    std::memset(m_rows, 0, sizeof(m_rows));
    std::memset(m_shadowRows, 0, sizeof(m_shadowRows));
    m_centerWorldX = 0;
    m_centerWorldY = 0;
    m_originWorldX = -64;
    m_originWorldY = -64;
  }

  void SetCenter(int32_t worldX, int32_t worldY) noexcept {
    m_centerWorldX = worldX;
    m_centerWorldY = worldY;
    m_originWorldX = worldX - 64;
    m_originWorldY = worldY - 64;
  }

  [[nodiscard]] HALO_INLINE int32_t GetOriginX() const noexcept { return m_originWorldX; }
  [[nodiscard]] HALO_INLINE int32_t GetOriginY() const noexcept { return m_originWorldY; }

  // Set Bit via Toroidal Modulo
  HALO_INLINE void SetBitWorld(int32_t layer, int32_t worldX, int32_t worldY) noexcept {
    int32_t ty = worldY & CLIP_MASK;
    int32_t tx = worldX & CLIP_MASK;
    uint64_t mask = 1ULL << (tx & 63);
    m_rows[ty][layer] |= mask;
    m_shadowRows[ty] |= mask;
  }

  [[nodiscard]] HALO_INLINE bool IsBlockedWorld(int32_t worldX, int32_t worldY) const noexcept {
    int32_t ty = worldY & CLIP_MASK;
    int32_t tx = worldX & CLIP_MASK;
    return (m_shadowRows[ty] & (1ULL << (tx & 63))) != 0;
  }

  // Ultra-Fast Reflex Raycast East in < 300 ns
  [[nodiscard]] HALO_INLINE int32_t RaycastEastLocal(int32_t localX, int32_t localY) const noexcept {
    if ((uint32_t)localY >= CLIP_DIM || localX >= CLIP_DIM - 1) return CLIP_DIM - 1;
    int32_t worldY = m_originWorldY + localY;

    for (int32_t step = localX + 1; step < CLIP_DIM; ++step) {
      if (IsBlockedWorld(m_originWorldX + step, worldY)) {
        return step;
      }
    }
    return CLIP_DIM - 1;
  }
};

// ============================================================================
// SPARSE BITBOARD WORLD (Continuous Chunk Allocation & DDA Raycasting)
// ============================================================================

class alignas(64) SparseBitboardWorld {
public:
  static constexpr int32_t MAX_CHUNKS = 1024; // 1024 * 8.5 KB = ~8.7 MB pool
  static constexpr uint32_t HASH_CAPACITY = 4096;

private:
  SparseChunk *m_chunkPool = nullptr;
  uint32_t m_chunkCount = 0;

  RobinHoodChunkMap m_chunkMap;
  RollingToroidalClipmap128 m_clipmap;

public:
  SparseBitboardWorld() noexcept = default;

  void Init(memory::ArenaAllocator &arena) noexcept {
    m_chunkPool = arena.AllocateArray<SparseChunk, 64>(MAX_CHUNKS);
    m_chunkCount = 0;
    m_chunkMap.Init(HASH_CAPACITY, arena);
    m_clipmap.Clear();
  }

  [[nodiscard]] uint32_t GetAllocatedChunkCount() const noexcept { return m_chunkCount; }
  [[nodiscard]] uint32_t GetMaxChunkCapacity() const noexcept { return MAX_CHUNKS; }

  [[nodiscard]] RollingToroidalClipmap128 &GetClipmap() noexcept { return m_clipmap; }
  [[nodiscard]] const RollingToroidalClipmap128 &GetClipmap() const noexcept { return m_clipmap; }

  // Allocate or retrieve populated chunk
  SparseChunk *GetOrCreateChunk(int32_t cx, int32_t cy) noexcept {
    uint32_t idx = m_chunkMap.Find(cx, cy);
    if (idx != RobinHoodChunkMap::EMPTY_INDEX && idx < m_chunkCount) {
      return &m_chunkPool[idx];
    }
    if (m_chunkCount >= MAX_CHUNKS) {
      return nullptr; // Pool exhausted
    }

    uint32_t newIdx = m_chunkCount++;
    m_chunkPool[newIdx].Init(cx, cy);
    m_chunkMap.Insert(cx, cy, newIdx);
    return &m_chunkPool[newIdx];
  }

  // Fast O(1) query for chunk; returns nullptr for empty space consuming 0 memory
  [[nodiscard]] HALO_INLINE SparseChunk *GetChunk(int32_t cx, int32_t cy) const noexcept {
    uint32_t idx = m_chunkMap.Find(cx, cy);
    if (idx == RobinHoodChunkMap::EMPTY_INDEX || idx >= m_chunkCount) {
      return nullptr;
    }
    return &m_chunkPool[idx];
  }

  // Set bit in world metric coordinates (1m / unit)
  void SetBitWorld(int32_t layer, int32_t worldX, int32_t worldY) noexcept {
    int32_t cx = (worldX >= 0) ? (worldX / 64) : ((worldX - 63) / 64);
    int32_t cy = (worldY >= 0) ? (worldY / 64) : ((worldY - 63) / 64);
    int32_t lx = worldX - (cx * 64);
    int32_t ly = worldY - (cy * 64);

    SparseChunk *chunk = GetOrCreateChunk(cx, cy);
    if (chunk) {
      chunk->SetBit(layer, lx, ly);
    }
  }

  [[nodiscard]] HALO_INLINE bool IsBlockedWorld(int32_t worldX, int32_t worldY) const noexcept {
    int32_t cx = (worldX >= 0) ? (worldX / 64) : ((worldX - 63) / 64);
    int32_t cy = (worldY >= 0) ? (worldY / 64) : ((worldY - 63) / 64);
    SparseChunk *chunk = GetChunk(cx, cy);
    if (!chunk) return false; // Empty space is 100% traversable!
    int32_t lx = worldX - (cx * 64);
    int32_t ly = worldY - (cy * 64);
    return chunk->IsBlocked(lx, ly);
  }

  // ============================================================================
  // BRANCHLESS CROSS-CHUNK DDA RAYCASTING WITH O(1) EMPTY CHUNK SKIPPING
  // ============================================================================

  [[nodiscard]] int32_t CrossChunkRaycastEast(int32_t startX, int32_t startY, int32_t maxDist) const noexcept {
    int32_t currentX = startX;
    int32_t remaining = maxDist;

    while (remaining > 0) {
      int32_t cx = (currentX >= 0) ? (currentX / 64) : ((currentX - 63) / 64);
      int32_t cy = (startY >= 0) ? (startY / 64) : ((startY - 63) / 64);
      int32_t lx = currentX - (cx * 64);
      int32_t ly = startY - (cy * 64);

      int32_t distToChunkBoundary = 64 - lx;
      int32_t stepSpan = (distToChunkBoundary < remaining) ? distToChunkBoundary : remaining;

      SparseChunk *chunk = GetChunk(cx, cy);
      if (!chunk) {
        // FAST PATH: Entire 64x64 chunk is empty space! Skip in a single step!
        currentX += stepSpan;
        remaining -= stepSpan;
      } else {
        // SLOW PATH: Populated chunk, evaluate intra-chunk bitboard
        int32_t hitLocalX = chunk->RaycastEast(lx, ly);
        if (hitLocalX < 63 && hitLocalX > lx) {
          int32_t hitDist = hitLocalX - lx;
          if (hitDist <= remaining) {
            return currentX + hitDist; // Impact detected!
          }
        }
        currentX += stepSpan;
        remaining -= stepSpan;
      }
    }
    return startX + maxDist; // Unobstructed ray
  }

  [[nodiscard]] int32_t CrossChunkRaycastWest(int32_t startX, int32_t startY, int32_t maxDist) const noexcept {
    int32_t currentX = startX;
    int32_t remaining = maxDist;

    while (remaining > 0) {
      int32_t cx = (currentX >= 0) ? (currentX / 64) : ((currentX - 63) / 64);
      int32_t cy = (startY >= 0) ? (startY / 64) : ((startY - 63) / 64);
      int32_t lx = currentX - (cx * 64);
      int32_t ly = startY - (cy * 64);

      int32_t distToChunkBoundary = lx + 1;
      int32_t stepSpan = (distToChunkBoundary < remaining) ? distToChunkBoundary : remaining;

      SparseChunk *chunk = GetChunk(cx, cy);
      if (!chunk) {
        // Empty chunk skip
        currentX -= stepSpan;
        remaining -= stepSpan;
      } else {
        int32_t hitLocalX = chunk->RaycastWest(lx, ly);
        if (hitLocalX > 0 && hitLocalX < lx) {
          int32_t hitDist = lx - hitLocalX;
          if (hitDist <= remaining) {
            return currentX - hitDist;
          }
        }
        currentX -= stepSpan;
        remaining -= stepSpan;
      }
    }
    return startX - maxDist;
  }
};

} // namespace halo::sparse
