#include <benchmark/benchmark.h>

#include <cmath>
#include <cstdint>
#include <vector>

#include "halo/core/halo_memory.h"
#include "halo/core/halo_omnicontext_core.h"
#include "halo/core/halo_supreme_core.h"
#include "halo/kinodynamics/halo_kinodynamics.h"
#include "halo/navigation/halo_jps_plus.h"
#include "halo/protection/halo_sparse_bitboard.h"
#include "halo/protection/halo_swar_10_layer_bitboard.h"
#include "halo/sensors/halo_sensor_fusion.h"
#include "halo/utils/halo_fixed_point.h"
#include "halo/utils/halo_heap.h"
#include "halo/utils/halo_math.h"

using namespace halo;
using namespace halo::fixed;

// ============================================================================
// 1. SWAR HARDWARE RAYCAST (0.34 ns / op target)
// ============================================================================
static void BM_SWAR_RaycastRow(benchmark::State &state) {
  uint64_t testRow = 0x8000400020001001ULL;
  int32_t startX = 0;
  uint64_t sink = 0;

  for (auto _ : state) {
#define R(offset) sink += omnicontext::AdaptiveOmniEngine::RaycastRow(testRow, (startX + offset) & 31)
    R(0);
    R(1);
    R(2);
    R(3);
    R(4);
    R(5);
    R(6);
    R(7);
    R(8);
    R(9);
    R(10);
    R(11);
    R(12);
    R(13);
    R(14);
    R(15);
    R(16);
    R(17);
    R(18);
    R(19);
    R(20);
    R(21);
    R(22);
    R(23);
    R(24);
    R(25);
    R(26);
    R(27);
    R(28);
    R(29);
    R(30);
    R(31);
#undef R
    startX++;
  }
  benchmark::DoNotOptimize(sink);
  state.SetItemsProcessed(state.iterations() * 32);
}
BENCHMARK(BM_SWAR_RaycastRow);

// ============================================================================
// 2. FIXED32 MATH: TRIGONOMETRIC LUT, SQRT & ATAN2 (Branchless Q16.16)
// ============================================================================
static void BM_Fixed32_SinCosDeg(benchmark::State &state) {
  int32_t deg = 0;
  Fixed32 sum(0);

  for (auto _ : state) {
    sum += SinDeg(deg) + CosDeg(deg);
    deg = (deg + 1) % 360;
  }
  benchmark::DoNotOptimize(sum);
  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Fixed32_SinCosDeg);

static void BM_Fixed32_IntegerSqrt(benchmark::State &state) {
  Fixed32 val(2);
  Fixed32 step = Fixed32::FromRaw(32768);  // +0.5
  Fixed32 sum(0);

  for (auto _ : state) {
    sum += Sqrt(val);
    val += step;
  }
  benchmark::DoNotOptimize(sum);
  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Fixed32_IntegerSqrt);

static void BM_Fixed32_Atan2(benchmark::State &state) {
  Fixed32 y = Fixed32::FromFloat(1.5f);
  Fixed32 x = Fixed32::FromFloat(2.5f);
  Fixed32 step = Fixed32::FromFloat(0.01f);
  Fixed32 sum(0);

  for (auto _ : state) {
    sum += Atan2(y, x);
    y += step;
  }
  benchmark::DoNotOptimize(sum);
  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Fixed32_Atan2);

// ============================================================================
// 3. 1 kHz PATH-FOLLOWING CONTROLLERS (Pure Pursuit & Stanley)
// ============================================================================
static void BM_Kinodynamics_PurePursuit(benchmark::State &state) {
  kinodynamics::KinodynamicLimits limits;
  limits.maxVelocity = 4.0f;
  limits.maxAcceleration = 5.0f;
  limits.nominalSpeed = 3.0f;

  Vec2f waypoints[4] = {{0.0f, 0.0f}, {20.0f, 0.0f}, {20.0f, 20.0f}, {40.0f, 20.0f}};

  kinodynamics::KinodynamicTrajectory traj;
  kinodynamics::GenerateQuinticTrajectory(waypoints, 4, limits, traj);

  Vec2f curPos{10.0f, 0.1f};
  float curHeading = 0.0f;

  for (auto _ : state) {
    auto cmd = kinodynamics::EvaluatePurePursuit(curPos, curHeading, traj, 2.0f, 1.0f, limits.nominalSpeed);
    benchmark::DoNotOptimize(cmd);
    curPos.x += 0.001f;
  }
  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Kinodynamics_PurePursuit);

static void BM_Kinodynamics_Stanley(benchmark::State &state) {
  kinodynamics::KinodynamicLimits limits;
  limits.maxVelocity = 4.0f;
  limits.maxAcceleration = 5.0f;
  limits.nominalSpeed = 3.0f;

  Vec2f waypoints[4] = {{0.0f, 0.0f}, {20.0f, 0.0f}, {20.0f, 20.0f}, {40.0f, 20.0f}};

  kinodynamics::KinodynamicTrajectory traj;
  kinodynamics::GenerateQuinticTrajectory(waypoints, 4, limits, traj);

  Vec2f curPos{10.0f, 0.1f};
  float curHeading = 0.0f;
  float curSpeed = 3.0f;

  for (auto _ : state) {
    auto cmd = kinodynamics::EvaluateStanley(curPos, curHeading, curSpeed, traj, 2.0f, 1.5f, limits.nominalSpeed);
    benchmark::DoNotOptimize(cmd);
    curPos.x += 0.001f;
  }
  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Kinodynamics_Stanley);

// ============================================================================
// 4. QUINTIC POLYNOMIAL SPLINE (Closed-Form Analytical Matrix Inversion)
// ============================================================================
static void BM_Kinodynamics_QuinticPoly1D(benchmark::State &state) {
  kinodynamics::QuinticPolynomial1D poly;
  float p0 = 0.0f, v0 = 0.0f, a0 = 0.0f;
  float p1 = 10.0f, v1 = 2.0f, a1 = 0.0f;
  float T = 2.5f;
  float sink = 0.0f;

  for (auto _ : state) {
    poly.Solve(p0, v0, a0, p1, v1, a1, T);
    sink += poly.Pos(T * 0.5f);
    p1 += 0.01f;
  }
  benchmark::DoNotOptimize(sink);
  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Kinodynamics_QuinticPoly1D);

static void BM_Kinodynamics_QuinticTrajectory4Pts(benchmark::State &state) {
  kinodynamics::KinodynamicLimits limits;
  limits.maxVelocity = 4.0f;
  limits.maxAcceleration = 5.0f;
  limits.nominalSpeed = 3.0f;

  Vec2f waypoints[4] = {{0.0f, 0.0f}, {20.0f, 0.0f}, {20.0f, 20.0f}, {40.0f, 20.0f}};

  kinodynamics::KinodynamicTrajectory traj;
  for (auto _ : state) {
    bool ok = kinodynamics::GenerateQuinticTrajectory(waypoints, 4, limits, traj);
    benchmark::DoNotOptimize(ok);
  }
  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Kinodynamics_QuinticTrajectory4Pts);

// Global arena and sparse bitboard world for sensor benchmarks
static memory::ArenaAllocator g_sensorArena(16 * 1024 * 1024);
static sparse::SparseBitboardWorld g_sensorWorld;
static bool g_sensorWorldInit = false;

static void EnsureSensorWorldInit() {
  if (!g_sensorWorldInit) {
    g_sensorWorld.Init(g_sensorArena);
    g_sensorWorldInit = true;
  }
}

// ============================================================================
// 5. SENSOR-POLYMORPHIC ZERO-COPY INGESTION PIPELINE
// ============================================================================
static void BM_SensorFusion_UltrasonicCone(benchmark::State &state) {
  EnsureSensorWorldInit();

  for (auto _ : state) {
    sensors::IngestRangeConeFixedPoint(g_sensorWorld, 0, 0, 45, 15, 8, swar::Layer::STATIC_WALLS);
  }
  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_SensorFusion_UltrasonicCone);

static void BM_SensorFusion_LiDAR360(benchmark::State &state) {
  EnsureSensorWorldInit();

  float lidarRanges[360];
  for (int i = 0; i < 360; ++i) {
    lidarRanges[i] = 4.0f + (i % 12) * 0.35f;
  }

  for (auto _ : state) {
    sensors::IngestLaserScanPolarSIMD(g_sensorWorld, lidarRanges, 360, 0.0f, 0.0174533f, 0.0f, 0.0f, 1.0f, 20.0f,
                                      swar::Layer::STATIC_WALLS);
  }
  state.SetItemsProcessed(state.iterations() * 360);
}
BENCHMARK(BM_SensorFusion_LiDAR360);

static void BM_SensorFusion_PointCloud10k(benchmark::State &state) {
  EnsureSensorWorldInit();

  std::vector<sensors::PointXYZ> cloud(10000);
  for (int i = 0; i < 10000; ++i) {
    cloud[i].x = static_cast<float>((i % 60) - 30);
    cloud[i].y = static_cast<float>((i / 60) - 30);
    cloud[i].z = 0.5f + static_cast<float>((i % 5) * 0.2f);
    cloud[i].intensity = 1.0f;
  }

  for (auto _ : state) {
    sensors::IngestPointCloudZeroCopy(g_sensorWorld, cloud.data(), 10000, 0.0f, 0.0f, 0.1f, 2.0f, 1.0f, swar::Layer::STATIC_WALLS);
  }
  state.SetItemsProcessed(state.iterations() * 10000);
}
BENCHMARK(BM_SensorFusion_PointCloud10k);

// ============================================================================
// 6. SPARSE BITBOARD WORLD CHUNK PROBE & SETBIT
// ============================================================================
static void BM_SparseBitboard_SetBit(benchmark::State &state) {
  EnsureSensorWorldInit();

  int64_t x = 0;
  int64_t y = 0;

  for (auto _ : state) {
    g_sensorWorld.SetBit(0, x & 1023, y & 1023);
    x += 3;
    y += 7;
  }
  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_SparseBitboard_SetBit);

// ============================================================================
// 7. JPS+ SUB-MICROSECOND ROUTING ON 512x512 GRID
// ============================================================================
static void BM_JPSPlus_512x512_Routing(benchmark::State &state) {
  constexpr int32_t MAP_DIM = 512;
  constexpr int32_t TOTAL_CELLS = MAP_DIM * MAP_DIM;

  static memory::ArenaAllocator jpsArena(16 * 1024 * 1024);
  static GridT<MAP_DIM, MAP_DIM> grid;
  static core::HaloSupremeEngineT<MAP_DIM, MAP_DIM> engine;
  static bool jpsInit = false;

  if (!jpsInit) {
    uint8_t *walkable = jpsArena.AllocateArray<uint8_t, 64>(TOTAL_CELLS);
    int32_t *penalty = jpsArena.AllocateArray<int32_t, 64>(TOTAL_CELLS);
    grid.Init(MAP_DIM, MAP_DIM, walkable, penalty);

    for (int32_t x = 0; x < MAP_DIM; ++x) {
      grid.SetObstacle(x, 0);
      grid.SetObstacle(x, MAP_DIM - 1);
    }
    for (int32_t y = 0; y < MAP_DIM; ++y) {
      grid.SetObstacle(0, y);
      grid.SetObstacle(MAP_DIM - 1, y);
    }
    for (int y = 50; y < 450; ++y) {
      grid.SetObstacle(160, y);
      grid.SetObstacle(320, y);
    }
    grid.SetWalkable(160, 250, true);
    grid.SetWalkable(320, 250, true);

    engine.BootSystem(&grid, nullptr, 64);
    jpsInit = true;
  }

  Vec2i start{40, 250};
  Vec2i target{450, 250};

  for (auto _ : state) {
    auto res = engine.RouteGridOptimal(start, target);
    benchmark::DoNotOptimize(res.found);
  }
  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_JPSPlus_512x512_Routing);

BENCHMARK_MAIN();
