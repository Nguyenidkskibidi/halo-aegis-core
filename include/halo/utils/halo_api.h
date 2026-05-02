#pragma once

#include <stdbool.h>
#include <stdint.h>

#if defined(_WIN32) || defined(_WIN64)
#define HALO_EXPORT __declspec(dllexport)
#else
#define HALO_EXPORT __attribute__((visibility("default")))
#endif

extern "C" {

struct HaloRouteResult {
  bool found;
  int32_t pathLength;
  int32_t waypointsX[1024];
  int32_t waypointsY[1024];
  float totalCost;
};

HALO_EXPORT void *Halo_InitSystem(int32_t memoryMB);

HALO_EXPORT void Halo_ShutdownSystem(void *systemPtr);

HALO_EXPORT void Halo_Grid_InitMap(void *systemPtr, int32_t width,
                                   int32_t height);
HALO_EXPORT void Halo_Grid_SetObstacle(void *systemPtr, int32_t x, int32_t y);
HALO_EXPORT void Halo_Grid_BakeClearance(void *systemPtr);

HALO_EXPORT HaloRouteResult Halo_Grid_FindPath(void *systemPtr, int32_t startX,
                                               int32_t startY, int32_t targetX,
                                               int32_t targetY);

HALO_EXPORT void Halo_Urban_InitCity(void *systemPtr, int32_t maxNodes,
                                     int32_t maxEdges);
HALO_EXPORT int32_t Halo_Urban_AddIntersection(void *systemPtr, float x,
                                               float y);
HALO_EXPORT void Halo_Urban_AddRoad(void *systemPtr, int32_t fromId,
                                    int32_t toId, float trafficJam);

HALO_EXPORT HaloRouteResult Halo_Urban_RouteTraffic(void *systemPtr,
                                                    int32_t startId,
                                                    int32_t targetId);
}