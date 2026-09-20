#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// BLITTABLE POD TYPES (UNREAL FVector, UNITY NativeArray, GODOT COMPATIBLE)
// ============================================================================

typedef struct {
  float x;
  float y;
} HaloVec2f;

typedef struct {
  float x;
  float y;
  float z;
} HaloVec3f;

typedef struct {
  int32_t x;
  int32_t y;
} HaloIntPoint;

typedef struct {
  int32_t q;
  int32_t r;
} HaloHexCoord;

typedef struct {
  int32_t found;
  int32_t count;
  HaloIntPoint waypoints[1024];
} HaloPathResult;

typedef struct {
  HaloVec2f pos;
  HaloVec2f vel;
  float maxSpeed;
  float radius;
  int32_t squadId;
  int32_t slotIdx;
  int32_t reachedGoal;
} HaloAgent;

// Opaque Engine Context
typedef struct HaloEngineContext HaloEngineContext;

// ============================================================================
// ENGINE LIFECYCLE & CONFIGURATION
// ============================================================================

HaloEngineContext *HaloCreateEngine(int32_t width, int32_t height, size_t arenaSizeMB);
void HaloDestroyEngine(HaloEngineContext *ctx);

void HaloSetObstacle(HaloEngineContext *ctx, int32_t x, int32_t y, int32_t isObstacle);
int32_t HaloIsObstacle(HaloEngineContext *ctx, int32_t x, int32_t y);

// ============================================================================
// ZERO-COPY PATH QUERY INTERFACES
// ============================================================================

int32_t HaloQueryPath(HaloEngineContext *ctx, HaloIntPoint start, HaloIntPoint goal, HaloPathResult *outResult);
int32_t HaloQueryPathOptimal(HaloEngineContext *ctx, HaloIntPoint start, HaloIntPoint goal, HaloPathResult *outResult);
int32_t HaloQueryPathAnyAngle(HaloEngineContext *ctx, HaloIntPoint start, HaloIntPoint goal, HaloPathResult *outResult);
int32_t HaloValidatePath(HaloEngineContext *ctx, const HaloPathResult *path);

int32_t HaloQueryRaycast(HaloEngineContext *ctx, HaloVec2f start, HaloVec2f dir, float maxDist, float *outClearance);

// ============================================================================
// CINEMATIC PATH SMOOTHING (ZERO-ALLOCATION)
// ============================================================================

int32_t HaloSmoothPathChaikin(const HaloVec2f *inPoints, int32_t inCount, HaloVec2f *outPoints, int32_t maxOut, int32_t iterations);

int32_t HaloSmoothPathCatmullRom(const HaloVec2f *inPoints, int32_t inCount, HaloVec2f *outPoints, int32_t maxOut, int32_t subdivisions);

#ifdef __cplusplus
}  // extern "C"

// Implementation of the C-ABI functions using halo engine internals
#include "../core/halo_memory.h"
#include "../core/halo_supreme_core.h"
#include "../navigation/halo_postprocess.h"
#include "../protection/halo_swar_10_layer_bitboard.h"
#include <new>

struct HaloEngineContext {
  halo::memory::ArenaAllocator arena;
  halo::GridT<512, 512> grid;
  halo::core::HaloSupremeEngineT<512, 512> engine;
  int32_t w = 512;
  int32_t h = 512;
};

inline HaloEngineContext *HaloCreateEngine(int32_t width, int32_t height, size_t arenaSizeMB) {
  void *mem = HALO_ALIGNED_ALLOC(sizeof(HaloEngineContext), 64);
  if (!mem) return nullptr;

  HaloEngineContext *ctx = new (mem) HaloEngineContext();
  ctx->w = width;
  ctx->h = height;
  ctx->arena.Init(4 * 1024 * 1024);

  uint8_t *walkBuf = ctx->arena.AllocateArray<uint8_t, 64>(static_cast<size_t>(512) * 512);
  int32_t *penBuf = ctx->arena.AllocateArray<int32_t, 64>(static_cast<size_t>(512) * 512);
  ctx->grid.Init(512, 512, walkBuf, penBuf);

  size_t engineMB = (arenaSizeMB >= 20) ? arenaSizeMB : 20;
  ctx->engine.BootSystem(&ctx->grid, engineMB);
  return ctx;
}

inline void HaloDestroyEngine(HaloEngineContext *ctx) {
  if (ctx) {
    ctx->arena.Release();
    ctx->~HaloEngineContext();
    HALO_ALIGNED_FREE(ctx);
  }
}

inline void HaloSetObstacle(HaloEngineContext *ctx, int32_t x, int32_t y, int32_t isObstacle) {
  if (ctx) {
    ctx->grid.SetWalkable(x, y, isObstacle == 0);
  }
}

inline int32_t HaloIsObstacle(HaloEngineContext *ctx, int32_t x, int32_t y) {
  if (!ctx) return 1;
  return ctx->grid.IsWalkable(x, y) ? 0 : 1;
}

inline int32_t HaloQueryPath(HaloEngineContext *ctx, HaloIntPoint start, HaloIntPoint goal, HaloPathResult *outResult) {
  if (!ctx || !outResult) return 0;

  halo::PathResult res = ctx->engine.RouteGrid(halo::Vec2i{start.x, start.y}, halo::Vec2i{goal.x, goal.y});
  outResult->found = res.found ? 1 : 0;
  outResult->count = res.len;
  for (int32_t i = 0; i < res.len && i < 1024; ++i) {
    outResult->waypoints[i].x = res.route[i].x;
    outResult->waypoints[i].y = res.route[i].y;
  }
  return res.found ? 1 : 0;
}

inline int32_t HaloQueryPathOptimal(HaloEngineContext *ctx, HaloIntPoint start, HaloIntPoint goal, HaloPathResult *outResult) {
  if (!ctx || !outResult) return 0;

  halo::PathResult res = ctx->engine.RouteGridOptimal(halo::Vec2i{start.x, start.y}, halo::Vec2i{goal.x, goal.y});
  outResult->found = res.found ? 1 : 0;
  outResult->count = res.len;
  for (int32_t i = 0; i < res.len && i < 1024; ++i) {
    outResult->waypoints[i].x = res.route[i].x;
    outResult->waypoints[i].y = res.route[i].y;
  }
  return res.found ? 1 : 0;
}

inline int32_t HaloQueryPathAnyAngle(HaloEngineContext *ctx, HaloIntPoint start, HaloIntPoint goal, HaloPathResult *outResult) {
  if (!ctx || !outResult) return 0;

  halo::ContinuousPathResult res = ctx->engine.RouteGridAnyAngle(halo::Vec2i{start.x, start.y}, halo::Vec2i{goal.x, goal.y});
  outResult->found = res.found ? 1 : 0;
  outResult->count = res.len;
  for (int32_t i = 0; i < res.len && i < 1024; ++i) {
    outResult->waypoints[i].x = static_cast<int32_t>(res.waypoints[i].x + 0.5f);
    outResult->waypoints[i].y = static_cast<int32_t>(res.waypoints[i].y + 0.5f);
  }
  return res.found ? 1 : 0;
}

inline int32_t HaloValidatePath(HaloEngineContext *ctx, const HaloPathResult *path) {
  if (!ctx || !path || !path->found || path->count <= 0) return 0;
  halo::PathResult p;
  p.found = true;
  p.len = std::min(path->count, 1024);
  for (int32_t i = 0; i < p.len; ++i) {
    p.route[i] = halo::Vec2i{path->waypoints[i].x, path->waypoints[i].y};
  }
  return ctx->engine.ValidatePathSafety(p) ? 1 : 0;
}

inline int32_t HaloQueryRaycast(HaloEngineContext *ctx, HaloVec2f start, HaloVec2f dir, float maxDist, float *outClearance) {
  if (!ctx || !outClearance) return 0;
  // Step along line to find first obstacle
  float step = 0.5f;
  int32_t numSteps = static_cast<int32_t>(maxDist / step);
  for (int32_t s = 1; s <= numSteps; ++s) {
    float dist = s * step;
    float px = start.x + dir.x * dist;
    float py = start.y + dir.y * dist;
    int32_t ix = static_cast<int32_t>(px + 0.5f);
    int32_t iy = static_cast<int32_t>(py + 0.5f);
    if (!ctx->grid.InBounds(ix, iy) || !ctx->grid.IsWalkable(ix, iy)) {
      *outClearance = (s - 1) * step;
      return 1;
    }
  }
  *outClearance = maxDist;
  return 1;
}

inline int32_t HaloSmoothPathChaikin(const HaloVec2f *inPoints, int32_t inCount, HaloVec2f *outPoints, int32_t maxOut, int32_t iterations) {
  return halo::postprocess::ChaikinSmooth(reinterpret_cast<const halo::Vec2f *>(inPoints), inCount,
                                          reinterpret_cast<halo::Vec2f *>(outPoints), maxOut, iterations);
}

inline int32_t HaloSmoothPathCatmullRom(const HaloVec2f *inPoints, int32_t inCount, HaloVec2f *outPoints, int32_t maxOut,
                                        int32_t subdivisions) {
  return halo::postprocess::CatmullRomSpline(reinterpret_cast<const halo::Vec2f *>(inPoints), inCount,
                                             reinterpret_cast<halo::Vec2f *>(outPoints), maxOut, subdivisions);
}

#endif  // __cplusplus
