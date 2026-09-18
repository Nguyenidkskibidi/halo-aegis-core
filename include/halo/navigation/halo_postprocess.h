#pragma once

#include "../utils/halo_types.h"
#include <algorithm>
#include <cmath>
#include <cstdint>

namespace halo::postprocess {

// ============================================================================
// 1. SIMPLE STUPID FUNNEL ALGORITHM (SSFA) & STRING PULLING
// ============================================================================

struct PortalVertex {
  Vec2f left{0.0f, 0.0f};
  Vec2f right{0.0f, 0.0f};
};

// Classical SSFA String Pulling through portal corridors with zero allocations
inline int32_t StringPullPortalsSSFA(const PortalVertex *portals, int32_t numPortals,
                                     Vec2f *outPath, int32_t maxOut) noexcept {
  if (!portals || numPortals <= 0 || !outPath || maxOut <= 0) return 0;

  int32_t outCount = 0;
  Vec2f portalApex = portals[0].left;
  Vec2f portalLeft = portals[0].left;
  Vec2f portalRight = portals[0].right;

  int32_t apexIndex = 0;
  int32_t leftIndex = 0;
  int32_t rightIndex = 0;

  outPath[outCount++] = portalApex;

  auto TriArea2 = [](Vec2f a, Vec2f b, Vec2f c) noexcept -> float {
    float ax = b.x - a.x;
    float ay = b.y - a.y;
    float bx = c.x - a.x;
    float by = c.y - a.y;
    return bx * ay - ax * by;
  };

  for (int32_t i = 1; i < numPortals; ++i) {
    Vec2f left = portals[i].left;
    Vec2f right = portals[i].right;

    // Update right leg of funnel
    if (TriArea2(portalApex, portalRight, right) <= 0.0f) {
      if (portalApex == portalRight || TriArea2(portalApex, portalLeft, right) > 0.0f) {
        // Tighten funnel
        portalRight = right;
        rightIndex = i;
      } else {
        // Right over left: new apex at left
        if (outCount < maxOut) outPath[outCount++] = portalLeft;
        portalApex = portalLeft;
        apexIndex = leftIndex;

        portalLeft = portalApex;
        portalRight = portalApex;
        leftIndex = apexIndex;
        rightIndex = apexIndex;

        i = apexIndex;
        continue;
      }
    }

    // Update left leg of funnel
    if (TriArea2(portalApex, portalLeft, left) >= 0.0f) {
      if (portalApex == portalLeft || TriArea2(portalApex, portalRight, left) < 0.0f) {
        // Tighten funnel
        portalLeft = left;
        leftIndex = i;
      } else {
        // Left over right: new apex at right
        if (outCount < maxOut) outPath[outCount++] = portalRight;
        portalApex = portalRight;
        apexIndex = rightIndex;

        portalLeft = portalApex;
        portalRight = portalApex;
        leftIndex = apexIndex;
        rightIndex = apexIndex;

        i = apexIndex;
        continue;
      }
    }
  }

  // Append final goal point
  if (outCount < maxOut) {
    outPath[outCount++] = portals[numPortals - 1].left;
  }
  return outCount;
}

// Raycast-based Line-of-Sight String Pulling for Grid Waypoints
template <typename RaycastClearFunc>
inline int32_t StringPullGridPath(const Vec2i *inPath, int32_t inCount,
                                  Vec2i *outPath, int32_t maxOut,
                                  RaycastClearFunc &&isLineClear) noexcept {
  if (!inPath || inCount <= 0 || !outPath || maxOut <= 0) return 0;
  if (inCount == 1) {
    outPath[0] = inPath[0];
    return 1;
  }

  int32_t outCount = 0;
  outPath[outCount++] = inPath[0];

  int32_t currentAnchor = 0;
  while (currentAnchor < inCount - 1) {
    int32_t furthestVisible = currentAnchor + 1;
    for (int32_t probe = inCount - 1; probe > currentAnchor + 1; --probe) {
      if (isLineClear(inPath[currentAnchor], inPath[probe])) {
        furthestVisible = probe;
        break;
      }
    }
    if (outCount < maxOut) {
      outPath[outCount++] = inPath[furthestVisible];
    } else {
      break;
    }
    currentAnchor = furthestVisible;
  }
  return outCount;
}

// ============================================================================
// 2. CHAIKIN'S CORNER CUTTING ALGORITHM (IN-PLACE ITERATIVE SMOOTHING)
// ============================================================================

inline int32_t ChaikinSmooth(const Vec2f *inPoints, int32_t inCount,
                             Vec2f *outPoints, int32_t maxOut,
                             int32_t iterations = 2) noexcept {
  if (!inPoints || inCount <= 0 || !outPoints || maxOut <= 0) return 0;
  if (inCount <= 2 || iterations <= 0) {
    int32_t n = std::min(inCount, maxOut);
    for (int32_t i = 0; i < n; ++i) outPoints[i] = inPoints[i];
    return n;
  }

  // Use a temporary scratch buffer if inPoints == outPoints or for multi-iteration
  constexpr int32_t SCRATCH_SIZE = 1024;
  Vec2f bufA[SCRATCH_SIZE];
  Vec2f bufB[SCRATCH_SIZE];

  int32_t curCount = std::min(inCount, SCRATCH_SIZE);
  for (int32_t i = 0; i < curCount; ++i) bufA[i] = inPoints[i];

  Vec2f *readBuf = bufA;
  Vec2f *writeBuf = bufB;

  for (int32_t it = 0; it < iterations; ++it) {
    int32_t writeCount = 0;
    if (writeCount < SCRATCH_SIZE) writeBuf[writeCount++] = readBuf[0]; // Retain start

    for (int32_t i = 0; i < curCount - 1; ++i) {
      Vec2f p0 = readBuf[i];
      Vec2f p1 = readBuf[i + 1];

      // Q = 0.75 * p0 + 0.25 * p1
      Vec2f q = p0 * 0.75f + p1 * 0.25f;
      // R = 0.25 * p0 + 0.75 * p1
      Vec2f r = p0 * 0.25f + p1 * 0.75f;

      if (writeCount < SCRATCH_SIZE) writeBuf[writeCount++] = q;
      if (writeCount < SCRATCH_SIZE) writeBuf[writeCount++] = r;
    }

    if (writeCount < SCRATCH_SIZE) writeBuf[writeCount++] = readBuf[curCount - 1]; // Retain goal

    curCount = writeCount;
    std::swap(readBuf, writeBuf);
  }

  int32_t finalCount = std::min(curCount, maxOut);
  for (int32_t i = 0; i < finalCount; ++i) {
    outPoints[i] = readBuf[i];
  }
  return finalCount;
}

// ============================================================================
// 3. CATMULL-ROM CUBIC SPLINE SMOOTHING
// ============================================================================

inline Vec2f EvaluateCatmullRom(Vec2f p0, Vec2f p1, Vec2f p2, Vec2f p3, float t) noexcept {
  float t2 = t * t;
  float t3 = t2 * t;

  return (p1 * 2.0f +
          (-p0 + p2) * t +
          (p0 * 2.0f - p1 * 5.0f + p2 * 4.0f - p3) * t2 +
          (-p0 + p1 * 3.0f - p2 * 3.0f + p3) * t3) * 0.5f;
}

inline int32_t CatmullRomSpline(const Vec2f *controlPoints, int32_t numControl,
                                Vec2f *outPoints, int32_t maxOut,
                                int32_t subdivisionsPerSegment = 4) noexcept {
  if (!controlPoints || numControl <= 0 || !outPoints || maxOut <= 0) return 0;
  if (numControl < 4) {
    int32_t n = std::min(numControl, maxOut);
    for (int32_t i = 0; i < n; ++i) outPoints[i] = controlPoints[i];
    return n;
  }

  int32_t outCount = 0;
  const float step = 1.0f / static_cast<float>(subdivisionsPerSegment);

  for (int32_t i = 0; i < numControl - 1; ++i) {
    Vec2f p0 = (i == 0) ? controlPoints[0] : controlPoints[i - 1];
    Vec2f p1 = controlPoints[i];
    Vec2f p2 = controlPoints[i + 1];
    Vec2f p3 = (i + 2 < numControl) ? controlPoints[i + 2] : p2;

    for (int32_t s = 0; s < subdivisionsPerSegment; ++s) {
      if (outCount >= maxOut) return outCount;
      float t = static_cast<float>(s) * step;
      outPoints[outCount++] = EvaluateCatmullRom(p0, p1, p2, p3, t);
    }
  }

  if (outCount < maxOut) {
    outPoints[outCount++] = controlPoints[numControl - 1];
  }
  return outCount;
}

// ============================================================================
// 4. KINEMATIC CURVATURE & TURN-RADIUS CLAMPING
// ============================================================================

inline int32_t CurvatureClamp(const Vec2f *inPoints, int32_t inCount,
                              Vec2f *outPoints, int32_t maxOut,
                              float maxTurnAngleRad = 1.04719755f) noexcept { // 60 deg max turn
  if (!inPoints || inCount <= 0 || !outPoints || maxOut <= 0) return 0;
  if (inCount <= 2) {
    int32_t n = std::min(inCount, maxOut);
    for (int32_t i = 0; i < n; ++i) outPoints[i] = inPoints[i];
    return n;
  }

  int32_t outCount = 0;
  outPoints[outCount++] = inPoints[0];

  Vec2f prevDir = (inPoints[1] - inPoints[0]).Normalized();
  outPoints[outCount++] = inPoints[1];

  for (int32_t i = 2; i < inCount; ++i) {
    if (outCount >= maxOut) break;

    Vec2f curDir = (inPoints[i] - outPoints[outCount - 1]).Normalized();
    float dot = std::clamp(prevDir.Dot(curDir), -1.0f, 1.0f);
    float angle = std::acos(dot);

    if (angle > maxTurnAngleRad) {
      // Clamp heading deviation: rotate prevDir by maxTurnAngleRad in the cross direction
      float cross = prevDir.Cross(curDir);
      float sign = (cross >= 0.0f) ? 1.0f : -1.0f;
      Vec2f clampedDir = prevDir.Rotated(sign * maxTurnAngleRad);
      float dist = (inPoints[i] - outPoints[outCount - 1]).Length();
      outPoints[outCount++] = outPoints[outCount - 1] + clampedDir * dist;
      prevDir = clampedDir;
    } else {
      outPoints[outCount++] = inPoints[i];
      prevDir = curDir;
    }
  }
  return outCount;
}

// ============================================================================
// 5. BACKWARD COMPATIBILITY RASTERIZATION
// ============================================================================

inline int32_t RasterizePathZeroAlloc(const Vec2i *waypoints, int32_t numWaypoints,
                                     Vec2i *outPath, int32_t maxOut) noexcept {
  if (!waypoints || numWaypoints <= 0 || !outPath || maxOut <= 0) return 0;

  int32_t count = 0;
  outPath[count++] = waypoints[0];

  for (int32_t i = 0; i < numWaypoints - 1; ++i) {
    Vec2i p0 = waypoints[i];
    Vec2i p1 = waypoints[i + 1];

    int32_t dx = std::abs(p1.x - p0.x);
    int32_t sx = (p0.x < p1.x) ? 1 : -1;
    int32_t dy = -std::abs(p1.y - p0.y);
    int32_t sy = (p0.y < p1.y) ? 1 : -1;
    int32_t err = dx + dy;

    while (true) {
      if (p0.x == p1.x && p0.y == p1.y) break;

      int32_t e2 = 2 * err;
      if (e2 >= dy) {
        err += dy;
        p0.x += sx;
      }
      if (e2 <= dx) {
        err += dx;
        p0.y += sy;
      }

      if (count < maxOut) {
        outPath[count++] = p0;
      } else {
        return count;
      }
    }
  }
  return count;
}

inline int32_t RasterizePath(const Vec2i *waypoints, int32_t count,
                             Vec2i *outPath, int32_t maxOut) noexcept {
  if (!waypoints || count <= 0 || !outPath || maxOut <= 0) return 0;
  int32_t outCount = 0;
  outPath[outCount++] = waypoints[0];

  for (int32_t i = 0; i < count - 1 && outCount < maxOut; ++i) {
    Vec2i p0 = waypoints[i];
    Vec2i p1 = waypoints[i + 1];

    int32_t dx = std::abs(p1.x - p0.x);
    int32_t sx = (p0.x < p1.x) ? 1 : -1;
    int32_t dy = -std::abs(p1.y - p0.y);
    int32_t sy = (p0.y < p1.y) ? 1 : -1;
    int32_t err = dx + dy;

    while (outCount < maxOut) {
      if (p0.x == p1.x && p0.y == p1.y) break;

      int32_t e2 = 2 * err;
      if (e2 >= dy) {
        err += dy;
        p0.x += sx;
      }
      if (e2 <= dx) {
        err += dx;
        p0.y += sy;
      }

      if (p0.x != p1.x || p0.y != p1.y) {
        outPath[outCount++] = p0;
      }
    }
    if (outCount < maxOut) {
      outPath[outCount++] = p1;
    }
  }
  return outCount;
}

} // namespace halo::postprocess