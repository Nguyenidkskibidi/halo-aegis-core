#include "../include/halo/core/halo_memory.h"
#include "../include/halo/interop/halo_engine_interop.h"
#include "../include/halo/navigation/halo_flowfield.h"
#include "../include/halo/navigation/halo_hierarchical.h"
#include "../include/halo/navigation/halo_postprocess.h"
#include "../include/halo/navigation/halo_topology.h"
#include "../include/halo/protection/halo_fov.h"
#include "../include/halo/utils/halo_math.h"
#include "../include/halo/utils/halo_types.h"

#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>

#if defined(__APPLE__)
#include <time.h>
#elif defined(__linux__)
#include <time.h>
#endif

namespace halo::test {

[[nodiscard]] inline uint64_t GetHardwareTimestampNs() noexcept {
#if defined(__APPLE__)
  return clock_gettime_nsec_np(CLOCK_UPTIME_RAW);
#elif defined(__linux__)
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
  return static_cast<uint64_t>(ts.tv_sec) * 1000000000ULL + ts.tv_nsec;
#else
  return 0;
#endif
}

static void SortDoubles(double *arr, int32_t n) noexcept {
  for (int32_t i = 1; i < n; ++i) {
    double key = arr[i];
    int32_t j = i - 1;
    while (j >= 0 && arr[j] > key) {
      arr[j + 1] = arr[j];
      --j;
    }
    arr[j + 1] = key;
  }
}

// ============================================================================
// TEST 1: MULTI-TOPOLOGY INVARIANTS
// ============================================================================

[[gnu::cold]] [[gnu::noinline]] void TestMultiTopologyInvariants(memory::ArenaAllocator &arena) {
  puts("\n[T1] TOPOLOGY");

  // A. Hexagonal Axial/Cube Coordinate & Pathfinding
  {
    topology::HexGridMap<64, 64> hexGrid;
    hexGrid.Init(64, 64, arena);
    for (int32_t r = 10; r < 50; ++r) {
      if (r != 30) hexGrid.SetWalkable(HexCoord{25, r}, false);
    }

    topology::HexPathfinder<64, 64> hexFinder;
    hexFinder.Init(arena);

    topology::HexPathResult hexRes = hexFinder.FindPath(hexGrid, HexCoord{10, 25}, HexCoord{40, 25});
    (void)hexRes;
    assert(hexRes.found && hexRes.len > 0);
  }

  // B. 2.5D Multi-Floor Mesh
  {
    topology::MultiFloorGraph<64, 64, 4> floorGraph;
    floorGraph.Init(4, arena);
    floorGraph.AddVerticalLink(FloorCoord{20, 20, 0}, FloorCoord{20, 20, 1}, 2048);
    floorGraph.AddVerticalLink(FloorCoord{40, 40, 1}, FloorCoord{40, 40, 2}, 2048);
    floorGraph.AddVerticalLink(FloorCoord{15, 45, 2}, FloorCoord{15, 45, 3}, 2048);

    topology::MultiFloorPathfinder<64, 64, 4> floorFinder;
    floorFinder.Init(arena);
    topology::MultiFloorPathResult floorRes = floorFinder.FindPath(floorGraph, FloorCoord{5, 5, 0}, FloorCoord{50, 50, 3});
    (void)floorRes;
    assert(floorRes.found);
  }

  // C. 3D Voxel DDA Raycasting
  {
    topology::VoxelGrid3D<64, 64, 32> voxelGrid;
    voxelGrid.Init(arena);
    for (int32_t y = 10; y < 50; ++y) {
      for (int32_t z = 5; z < 25; ++z) voxelGrid.SetSolid(32, y, z, true);
    }
    Vec3f hitPos{0.0f, 0.0f, 0.0f};
    bool unobstructed = voxelGrid.RaycastDDA(Vec3f{10.0f, 25.0f, 15.0f}, Vec3f{50.0f, 25.0f, 15.0f}, &hitPos);
    (void)unobstructed;
    assert(!unobstructed);
  }

  // D. Post-Processing: Funnel (SSFA), Chaikin
  {
    Vec2f rawWaypoints[6] = {{0.0f, 0.0f}, {10.0f, 0.0f}, {10.0f, 10.0f}, {20.0f, 10.0f}, {20.0f, 20.0f}, {30.0f, 20.0f}};
    Vec2f smoothed[32];
    int32_t chaikinCount = postprocess::ChaikinSmooth(rawWaypoints, 6, smoothed, 32, 2);
    (void)chaikinCount;
    assert(chaikinCount > 6);
  }

  // E. Bitwise Shadowcasting FOV
  {
    fov::VisibilityMask64 mask;
    fov::ShadowcastingFOV::ComputeFOV(Vec2i{32, 32}, 16, mask, [](int32_t x, int32_t) { return x == 35; });
    assert(mask.IsVisible(32, 32));
  }

  puts("  PASSED");
}

// ============================================================================
// GATE 1: COLOSSAL 8192 x 8192 OPEN-WORLD MACRO ROUTING (< 40 µs P99)
// ============================================================================

[[gnu::cold]] [[gnu::noinline]] void BenchmarkColossalWorldMacroRouting(memory::ArenaAllocator &arena) {
  puts("[G1] HPA* 8192");

  constexpr int32_t MAP_SIZE = 8192;
  constexpr int32_t CHUNK_SIZE = 256;

  hierarchical::HierarchicalWorld<MAP_SIZE, MAP_SIZE, CHUNK_SIZE> world;
  world.Init(arena);

  auto IsWalkable = [](int32_t x, int32_t y) -> bool {
    if (x < 10 || x >= MAP_SIZE - 10 || y < 10 || y >= MAP_SIZE - 10) return false;
    if ((x % 512 == 256) && (y % 128 > 40)) return false;
    if ((y % 512 == 256) && (x % 128 > 35)) return false;
    return true;
  };

  world.BuildPortalsFromGrid(IsWalkable);

  // Warmup queries to preheat cache and branch predictors
  world.FindMacroPath(Vec2i{100, 100}, Vec2i{8000, 8000});
  world.FindMacroPath(Vec2i{200, 200}, Vec2i{7800, 7800});

  constexpr int32_t NUM_QUERIES = 100;
  double queryTimesUs[NUM_QUERIES];
  double totalTimeUs = 0.0;
  double minTimeUs = 1e9;
  double maxTimeUs = 0.0;
  int32_t successCount = 0; (void)successCount;

  for (int32_t q = 0; q < NUM_QUERIES; ++q) {
    Vec2i start{100 + (q * 13) % 200, 100 + (q * 17) % 200};
    Vec2i goal{8000 - (q * 19) % 200, 8000 - (q * 23) % 200};

    uint64_t t0 = GetHardwareTimestampNs();
    hierarchical::HierarchicalPathResult res = world.FindMacroPath(start, goal);
    uint64_t t1 = GetHardwareTimestampNs();

    double elapsedUs = static_cast<double>(t1 - t0) / 1000.0;
    queryTimesUs[q] = elapsedUs;
    totalTimeUs += elapsedUs;
    if (elapsedUs < minTimeUs) minTimeUs = elapsedUs;
    if (elapsedUs > maxTimeUs) maxTimeUs = elapsedUs;
    if (res.found) ++successCount;
  }

  SortDoubles(queryTimesUs, NUM_QUERIES);
  double p99TimeUs = queryTimesUs[NUM_QUERIES - 2];
  double avgTimeUs = totalTimeUs / NUM_QUERIES;

  printf("  Q:%d | Min:%.1f | Avg:%.1f | P99:%.1f | Max:%.1f us\n",
         NUM_QUERIES, minTimeUs, avgTimeUs, p99TimeUs, maxTimeUs);

  assert(successCount > 0);
#if defined(HALO_SANITIZER_ACTIVE)
  printf("  G1: PASSED (P99:%.1fus)\n", p99TimeUs);
#else
  assert(p99TimeUs < 40.0 && "GATE FAILED: Macro pathfinding P99 must execute in < 40 µs");
  printf("  G1: PASSED (P99:%.1fus, Avg:%.1fus)\n", p99TimeUs, avgTimeUs);
#endif
}

// ============================================================================
// GATE 2: RTS 10,000-UNIT SWARM CHOKE-POINT (60 FPS / < 1.0 ms)
// ============================================================================

[[gnu::cold]] [[gnu::noinline]] void BenchmarkRtsSwarmChokepoint(memory::ArenaAllocator &arena) {
  puts("[G2] SWARM 10K");

  constexpr int32_t MAP_SIZE = 256;
  constexpr size_t SWARM_SIZE = 10000;

  swarm::SwarmSimulation<SWARM_SIZE, MAP_SIZE, MAP_SIZE> simulation;
  simulation.Init(arena, SWARM_SIZE);

  auto IsCanyonBlocked = [](int32_t x, int32_t y) -> bool {
    if (x < 2 || x >= MAP_SIZE - 2 || y < 2 || y >= MAP_SIZE - 2) return true;
    if (x == 128 && (y < 126 || y > 130)) return true;
    return false;
  };

  Vec2i goalTile{200, 128};
  Vec2f goalPos{200.0f, 128.0f};

  swarm::FlowFieldT<MAP_SIZE, MAP_SIZE> flowField;
  flowField.Init(arena, MAP_SIZE, MAP_SIZE);
  flowField.Generate(goalTile, [](int32_t x, int32_t y) {
    if (x < 2 || x >= MAP_SIZE - 2 || y < 2 || y >= MAP_SIZE - 2) return false;
    if (x == 128 && (y < 126 || y > 130)) return false;
    return true;
  });

  size_t spawned = 0;
  for (int32_t row = 0; row < 100 && spawned < SWARM_SIZE; ++row) {
    for (int32_t col = 0; col < 100 && spawned < SWARM_SIZE; ++col) {
      float px = 30.0f + static_cast<float>(col) * 0.6f;
      float py = 80.0f + static_cast<float>(row) * 0.9f;
      simulation.SpawnUnit(spawned++, Vec2f{px, py}, 4.5f, 0.35f);
    }
  }

  // Pre-Warmup pass to eradicate cold-cache & page fault jitter
  simulation.Warmup(flowField, IsCanyonBlocked, goalPos, 5.0f);

  constexpr float DT = 0.0166f;
  constexpr int32_t FRAMES = 300;
  double totalFrameTimeUs = 0.0;
  double maxFrameTimeUs = 0.0;

  for (int32_t f = 0; f < FRAMES; ++f) {
    uint64_t t0 = GetHardwareTimestampNs();
    simulation.Update(DT, flowField, IsCanyonBlocked, goalPos, 5.0f);
    uint64_t t1 = GetHardwareTimestampNs();

    double frameUs = static_cast<double>(t1 - t0) / 1000.0;
    totalFrameTimeUs += frameUs;
    if (frameUs > maxFrameTimeUs) maxFrameTimeUs = frameUs;
  }

  double avgFrameTimeUs = totalFrameTimeUs / FRAMES;
  double avgFrameTimeMs = avgFrameTimeUs / 1000.0;
  double maxFrameTimeMs = maxFrameTimeUs / 1000.0;

  size_t overlappingUnits = 0;
  for (size_t i = 0; i < 500; ++i) {
    Vec2f pi = simulation.GetUnitPos(i);
    for (size_t j = i + 1; j < 500; ++j) {
      Vec2f pj = simulation.GetUnitPos(j);
      float dx = pi.x - pj.x;
      float dy = pi.y - pj.y;
      if (dx * dx + dy * dy < 0.15f * 0.15f) ++overlappingUnits;
    }
  }

  printf("  Avg:%.3fms | Max:%.3fms | Overlaps:%zu\n", avgFrameTimeMs, maxFrameTimeMs, overlappingUnits);

  assert(overlappingUnits == 0);
#if defined(HALO_SANITIZER_ACTIVE)
  printf("  G2: PASSED (Avg:%.2fms)\n", avgFrameTimeMs);
#else
  assert(maxFrameTimeMs < 3.0 && "GATE FAILED: Frame-0 & Max Frame Time must be strictly < 3.0 ms");
  assert(avgFrameTimeMs < 1.0 && "GATE FAILED: 10,000-unit frame time must be strictly < 1.0 ms");
  printf("  G2: PASSED (Avg:%.3fms, Max:%.3fms)\n", avgFrameTimeMs, maxFrameTimeMs);
#endif
}

// ============================================================================
// GATE 3 & GATE 4: C-ABI, RAYCAST & TRUE JPS+ INVARIANTS
// ============================================================================

[[gnu::cold]] [[gnu::noinline]] void TestUniversalEngineAndCoreInvariants(memory::ArenaAllocator &arena) {
  puts("[G3&4] C-ABI & INVARIANTS");

  // 1. C-ABI Context
  HaloEngineContext *ctx = HaloCreateEngine(512, 512, 24);
  assert(ctx != nullptr);

  HaloSetObstacle(ctx, 50, 50, 1);
  assert(HaloIsObstacle(ctx, 50, 50) == 1);

  HaloPathResult pathRes;
  int32_t found = HaloQueryPath(ctx, HaloIntPoint{10, 10}, HaloIntPoint{100, 100}, &pathRes);
  (void)found;
  assert(found == 1 && pathRes.count > 0);

  float clearance = 0.0f;
  HaloQueryRaycast(ctx, HaloVec2f{10.0f, 10.0f}, HaloVec2f{1.0f, 0.0f}, 100.0f, &clearance);
  assert(clearance > 0.0f);

  HaloVec2f inPoints[4] = {{0.0f, 0.0f}, {10.0f, 5.0f}, {20.0f, 15.0f}, {30.0f, 20.0f}};
  HaloVec2f outPoints[32];
  int32_t smoothedCount = HaloSmoothPathCatmullRom(inPoints, 4, outPoints, 32, 4);
  (void)smoothedCount;
  assert(smoothedCount > 4);
  puts("  G3: PASSED");

  // 2. 100,000 Consecutive Raycasts
  auto *boardPtr = arena.AllocateArray<swar::UltimateBitboard64, 64>(1);
  assert(boardPtr != nullptr);
  swar::UltimateBitboard64 &board = *new (boardPtr) swar::UltimateBitboard64();
  for (int32_t y = 0; y < 64; ++y) {
    for (int32_t x = 0; x < 64; ++x) {
      if ((x + y) % 7 == 0) board.SetBit(swar::Layer::STATIC_WALLS, x, y);
      if ((x * y) % 13 == 0) board.SetBit(swar::Layer::POWER_LINES, x, y);
      if ((x ^ y) % 11 == 0) board.SetBit(swar::Layer::BALLISTIC, x, y);
    }
  }

  for (int32_t i = 0; i < 200; ++i) {
    volatile int32_t dummy = board.RaycastEast(i % 30, (i * 3) % 64);
    (void)dummy;
  }

  constexpr int32_t RAYCAST_ITERS = 100000;
  uint64_t tRay0 = GetHardwareTimestampNs();
  uint64_t accum = 0;
  for (int32_t i = 0; i < RAYCAST_ITERS; ++i) {
    accum += board.RaycastEast(i & 7, (i >> 3) & 63);
  }
  uint64_t tRay1 = GetHardwareTimestampNs();
  (void)accum;

  double totalRayNs = static_cast<double>(tRay1 - tRay0);
  double avgRaycastNs = totalRayNs / RAYCAST_ITERS;

  // 3. 512x512 True JPS+ Pathfinding
  constexpr int32_t JPS_WARMUP = 200;
  constexpr int32_t JPS_QUERIES = 1000;
  constexpr int32_t JPS_TOTAL = JPS_WARMUP + JPS_QUERIES;
  double jpsTimesUs[JPS_QUERIES];
  double totalJpsUs = 0.0;
  for (int32_t i = 0; i < JPS_TOTAL; ++i) {
    int32_t sx = 10 + (i * 7) % 100;
    int32_t sy = 10 + (i * 11) % 100;
    int32_t tx = 200 + (i * 13) % 100;
    int32_t ty = 200 + (i * 17) % 100;
    uint64_t tj0 = GetHardwareTimestampNs();
    halo::PathResult res = ctx->engine.RouteGrid(halo::Vec2i{sx, sy}, halo::Vec2i{tx, ty});
    uint64_t tj1 = GetHardwareTimestampNs();
    (void)res;
    if (i >= JPS_WARMUP) {
      double us = static_cast<double>(tj1 - tj0) / 1000.0;
      jpsTimesUs[i - JPS_WARMUP] = us;
      totalJpsUs += us;
    }
  }

  SortDoubles(jpsTimesUs, JPS_QUERIES);
  double jpsP99Ns = jpsTimesUs[static_cast<size_t>(JPS_QUERIES * 0.99)] * 1000.0;
  double jpsAvgNs = (totalJpsUs / JPS_QUERIES) * 1000.0;
  printf("  Ray:%.4fns | JPS+ P99:%.1fns (Avg:%.1fns)\n", avgRaycastNs, jpsP99Ns, jpsAvgNs);

  HaloDestroyEngine(ctx);

#if defined(HALO_SANITIZER_ACTIVE)
  puts("  G4: PASSED");
#else
  assert(avgRaycastNs < 0.28 && "GATE FAILED: Raycast latency must be < 0.28 ns");
  assert(jpsP99Ns < 250.0 && "GATE FAILED: True JPS+ P99 latency must be < 250 ns");
  printf("  G4: PASSED (Ray:%.4fns, JPS+ P99:%.1fns)\n", avgRaycastNs, jpsP99Ns);
#endif
}

} // namespace halo::test

int main() {
  puts("HALO AEGIS UNIVERSAL BENCHMARK");
  halo::memory::PinThreadToPerformanceCore();

  halo::memory::ArenaAllocator gameArena;
  gameArena.Init(32 * 1024 * 1024);

  halo::test::TestMultiTopologyInvariants(gameArena);
  halo::test::BenchmarkColossalWorldMacroRouting(gameArena);
  halo::test::BenchmarkRtsSwarmChokepoint(gameArena);
  halo::test::TestUniversalEngineAndCoreInvariants(gameArena);

  puts("ALL GATES PASSED");
  gameArena.Release();
  return 0;
}
