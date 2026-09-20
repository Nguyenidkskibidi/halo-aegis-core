#include "halo/interop/halo_engine_interop.h"

#if defined(_WIN32) || defined(__CYGWIN__)
#define HALO_EXPORT __declspec(dllexport)
#else
#define HALO_EXPORT __attribute__((visibility("default")))
#endif

extern "C" {

HALO_EXPORT HaloEngineContext *ExportHaloCreateEngine(int32_t width, int32_t height, size_t arenaSizeMB) {
  return HaloCreateEngine(width, height, arenaSizeMB);
}

HALO_EXPORT void ExportHaloDestroyEngine(HaloEngineContext *ctx) {
  HaloDestroyEngine(ctx);
}

HALO_EXPORT void ExportHaloSetObstacle(HaloEngineContext *ctx, int32_t x, int32_t y, int32_t isObstacle) {
  HaloSetObstacle(ctx, x, y, isObstacle);
}

HALO_EXPORT int32_t ExportHaloIsObstacle(HaloEngineContext *ctx, int32_t x, int32_t y) {
  return HaloIsObstacle(ctx, x, y);
}

HALO_EXPORT int32_t ExportHaloQueryPath(HaloEngineContext *ctx, HaloIntPoint start, HaloIntPoint goal, HaloPathResult *outResult) {
  return HaloQueryPath(ctx, start, goal, outResult);
}

HALO_EXPORT int32_t ExportHaloQueryPathOptimal(HaloEngineContext *ctx, HaloIntPoint start, HaloIntPoint goal, HaloPathResult *outResult) {
  return HaloQueryPathOptimal(ctx, start, goal, outResult);
}

HALO_EXPORT int32_t ExportHaloQueryPathAnyAngle(HaloEngineContext *ctx, HaloIntPoint start, HaloIntPoint goal, HaloPathResult *outResult) {
  return HaloQueryPathAnyAngle(ctx, start, goal, outResult);
}

HALO_EXPORT int32_t ExportHaloQueryRaycast(HaloEngineContext *ctx, HaloVec2f start, HaloVec2f dir, float maxDist, float *outClearance) {
  return HaloQueryRaycast(ctx, start, dir, maxDist, outClearance);
}

HALO_EXPORT int32_t ExportHaloSmoothPathCatmullRom(const HaloVec2f *inPoints, int32_t inCount, HaloVec2f *outPoints, int32_t maxOut,
                                                   int32_t subdivisions) {
  return HaloSmoothPathCatmullRom(inPoints, inCount, outPoints, maxOut, subdivisions);
}

}  // extern "C"
