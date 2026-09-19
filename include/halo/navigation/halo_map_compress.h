#pragma once

#include "../protection/halo_sparse_bitboard.h"
#include <cmath>
#include <cstdint>

namespace halo::streaming {

// RLE Compressed Spatial Span Packet for In-Place Decoding
struct alignas(8) RLESpatialSpan {
  int32_t worldY = 0;
  int32_t startX = 0;
  int32_t endX = 0;
  uint8_t layer = 0;
};

// Circular No-Fly / Threat Cylinder Zone
struct alignas(8) CircularExclusionZone {
  int32_t centerX = 0;
  int32_t centerY = 0;
  int32_t radius = 0;
  uint8_t layer = 0;
};

// ============================================================================
// ZERO-ALLOCATION STREAMING INGEST & COMPRESSION PIPELINE
// ============================================================================

class SpatialRasterIngestor {
public:
  // Decodes RLE spans directly into active sparse chunks
  static void IngestSpans(sparse::SparseBitboardWorld &world,
                          const RLESpatialSpan *spans,
                          size_t spanCount) noexcept {
    for (size_t i = 0; i < spanCount; ++i) {
      const auto &span = spans[i];
      for (int32_t x = span.startX; x <= span.endX; ++x) {
        world.SetBitWorld(span.layer, x, span.worldY);
      }
    }
  }

  // Rasterizes circular aviation exclusion cones (Airports, Heliports, Radar)
  static void IngestCircularZone(sparse::SparseBitboardWorld &world,
                                const CircularExclusionZone &zone) noexcept {
    int32_t r = zone.radius;
    int32_t rSq = r * r;

    for (int32_t dy = -r; dy <= r; ++dy) {
      int32_t y = zone.centerY + dy;
      int32_t dxMax = static_cast<int32_t>(std::sqrt(rSq - dy * dy));
      for (int32_t dx = -dxMax; dx <= dxMax; ++dx) {
        world.SetBitWorld(zone.layer, zone.centerX + dx, y);
      }
    }
  }

  // Rasterizes rectangular urban buildings and obstacles
  static void IngestRectangularBlock(sparse::SparseBitboardWorld &world,
                                    int32_t minX, int32_t minY,
                                    int32_t maxX, int32_t maxY,
                                    uint8_t layer) noexcept {
    for (int32_t y = minY; y <= maxY; ++y) {
      for (int32_t x = minX; x <= maxX; ++x) {
        world.SetBitWorld(layer, x, y);
      }
    }
  }
};

// ============================================================================
// DYNAMIC HORIZON STREAMER & TOROIDAL CLIPMAP CONTROLLER
// ============================================================================

class DynamicHorizonStreamer {
private:
  int32_t m_lastVehicleX = 0;
  int32_t m_lastVehicleY = 0;
  int32_t m_horizonRadius = 2000; // 2.0 km dynamic active horizon

public:
  constexpr DynamicHorizonStreamer() noexcept = default;

  void Init(int32_t initialX, int32_t initialY, int32_t horizonRadiusMeters = 2000) noexcept {
    m_lastVehicleX = initialX;
    m_lastVehicleY = initialY;
    m_horizonRadius = horizonRadiusMeters;
  }

  // Synchronizes rolling toroidal clipmap with current vehicle position
  void UpdateVehiclePosition(sparse::SparseBitboardWorld &world,
                             int32_t vehicleX, int32_t vehicleY) noexcept {
    m_lastVehicleX = vehicleX;
    m_lastVehicleY = vehicleY;

    // Update the 128x128 1-meter rolling toroidal clipmap center
    auto &clipmap = world.GetClipmap();
    clipmap.SetCenter(vehicleX, vehicleY);

    // Populate active clipmap window directly from surrounding sparse chunks
    int32_t originX = clipmap.GetOriginX();
    int32_t originY = clipmap.GetOriginY();

    for (int32_t ly = 0; ly < 128; ly += 8) {
      for (int32_t lx = 0; lx < 128; lx += 8) {
        int32_t wx = originX + lx;
        int32_t wy = originY + ly;
        if (world.IsBlockedWorld(wx, wy)) {
          clipmap.SetBitWorld(0, wx, wy);
        }
      }
    }
  }

  [[nodiscard]] int32_t GetHorizonRadius() const noexcept { return m_horizonRadius; }
};

} // namespace halo::streaming
