#include <algorithm>
#include <cassert>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <numeric>
#include <vector>

#include "halo/core/halo_memory.h"
#include "halo/core/halo_simd.h"
#include "halo/core/halo_supreme_core.h"
#include "halo/navigation/halo_flight_core.h"
#include "halo/protection/halo_swar_10_layer_bitboard.h"
#include "halo/utils/halo_heap.h"
#include "halo/utils/halo_math.h"
#include "halo/utils/halo_types.h"

namespace halo::test {

#if defined(__SANITIZE_ADDRESS__)
#define HALO_SANITIZER_ACTIVE 1
#elif defined(__has_feature)
#if __has_feature(address_sanitizer) || __has_feature(thread_sanitizer) || __has_feature(undefined_behavior_sanitizer)
#define HALO_SANITIZER_ACTIVE 1
#endif
#endif

// High-Precision Hardware Nanosecond Clock
[[nodiscard]] HALO_INLINE uint64_t GetHardwareNanos() noexcept {
#if defined(__APPLE__)
  return clock_gettime_nsec_np(CLOCK_UPTIME_RAW);
#elif defined(__linux__)
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
  return static_cast<uint64_t>(ts.tv_sec) * 1000000000ULL + ts.tv_nsec;
#else
  return static_cast<uint64_t>(
      std::chrono::duration_cast<std::chrono::nanoseconds>(
          std::chrono::steady_clock::now().time_since_epoch())
          .count());
#endif
}

// Generate wide corridor flight labyrinth with alternating gates
template <typename GridType, typename MatrixType>
[[gnu::cold]] void BuildFlightMaze(GridType &grid, MatrixType &matrix, int32_t w, int32_t h) {
  // 1. Boundary walls
  for (int32_t x = 0; x < w; ++x) {
    grid.SetObstacle(x, 0);
    grid.SetObstacle(x, h - 1);
    matrix.SetBit(swar::Layer::STATIC_WALLS, x, 0);
    matrix.SetBit(swar::Layer::STATIC_WALLS, x, h - 1);
  }
  for (int32_t y = 0; y < h; ++y) {
    grid.SetObstacle(0, y);
    grid.SetObstacle(w - 1, y);
    matrix.SetBit(swar::Layer::STATIC_WALLS, 0, y);
    matrix.SetBit(swar::Layer::STATIC_WALLS, w - 1, y);
  }

  // 2. Wide horizontal corridor barriers (spacing 20 tiles, gate width 8 tiles)
  for (int32_t y = 20; y < h - 20; y += 24) {
    bool leftSide = ((y / 24) % 2 == 0);
    int32_t gateX = leftSide ? 25 : (w - 35);

    for (int32_t x = 10; x < w - 10; ++x) {
      if (x < gateX || x > gateX + 8) {
        grid.SetObstacle(x, y);
        matrix.SetBit(swar::Layer::STATIC_WALLS, x, y);
      }
    }
  }

  // 3. Isolated static radar/power line pillars
  for (int32_t y = 12; y < h - 12; y += 24) {
    for (int32_t x = 30; x < w - 30; x += 40) {
      grid.SetObstacle(x, y);
      matrix.SetBit(swar::Layer::STATIC_WALLS, x, y);
      matrix.SetBit(swar::Layer::POWER_LINES, x, y);
    }
  }
}

void RunDynamicFlightSimulation() {
  puts("[FLIGHT] EMBEDDED DRONE BENCHMARK");
  bool pinned = memory::PinThreadToPerformanceCore(0);
  (void)pinned;

  constexpr int32_t MAP_W = 512;
  constexpr int32_t MAP_H = 512;
  constexpr int32_t TOTAL_CELLS = MAP_W * MAP_H;

  // Strict Embedded SWaP-C Memory Verification (<= 16 MB)
  memory::ArenaAllocator envArena(1024 * 1024);
  uint8_t *walk = envArena.AllocateArray<uint8_t, 64>(TOTAL_CELLS);

  GridT<MAP_W, MAP_H> grid;
  grid.Init(MAP_W, MAP_H, walk, nullptr);

  swar::LayeredHazardMatrixT<MAP_W, MAP_H> hazardMatrix;
  hazardMatrix.Init(MAP_W, MAP_H, envArena);

  BuildFlightMaze(grid, hazardMatrix, MAP_W, MAP_H);

  constexpr size_t SUPREME_RAM_MB = 15;
  core::HaloSupremeEngineT<MAP_W, MAP_H> supremeEngine;
  supremeEngine.BootSystem(&grid, nullptr, SUPREME_RAM_MB);

  size_t totalMemoryBytes = envArena.GetCapacity() + supremeEngine.GetMasterArenaCapacity();
  (void)totalMemoryBytes;
  assert(totalMemoryBytes <= 16 * 1024 * 1024 && "GATE FAILED: Total memory must not exceed 16 MB");

  // Precompute Global Macro Path (Tier 1 True JPS+)
  Vec2i startTile(15, 10);
  Vec2i goalTile(495, 495);

  PathResult macroRoute = supremeEngine.RouteGrid(startTile, goalTile);
  assert(macroRoute.found && "Macro path must be found across flight maze");

  // Initialize 500 Moving Obstacle Swarm
  flight::DynamicObstacleSwarm<500> swarm;
  swarm.Init(MAP_W, MAP_H, grid, 9999);

  // Initialize Hierarchical Flight Engine
  flight::HierarchicalFlightEngine<MAP_W, MAP_H> flightEngine;
  flightEngine.InitFlight(Vec2f(static_cast<float>(startTile.x), static_cast<float>(startTile.y)),
                          Vec2f(static_cast<float>(goalTile.x), static_cast<float>(goalTile.y)),
                          macroRoute);

  // Execute Closed-Loop 100 Hz Flight Simulation (1,000 meters / 1,000+ steps)
  constexpr float DT = 0.01f;
  constexpr size_t MIN_STEPS = 1000;
  size_t stepCount = 0;
  size_t collisionCount = 0;
  size_t deadlineMisses = 0;
  size_t evasionActiveCount = 0;

  double totalCycleTimeNs = 0.0;
  double maxCycleTimeNs = 0.0;
  double totalEvasionLatencyNs = 0.0;
  double maxEvasionLatencyNs = 0.0;
  float minObsClearance = 999.0f;
  float totalDistanceTraversed = 0.0f;

  while (stepCount < MIN_STEPS || !flightEngine.HasReachedGoal(5.0f)) {
    swarm.Update(DT, hazardMatrix, grid);
    flight::FlightTelemetry telem = flightEngine.StepControlCycle(DT, hazardMatrix, &swarm);
    ++stepCount;

    const auto &drone = flightEngine.GetDrone();
    totalDistanceTraversed += drone.vel.Length() * DT;

    totalCycleTimeNs += static_cast<double>(telem.cycleTimeNs);
    maxCycleTimeNs = std::max(maxCycleTimeNs, static_cast<double>(telem.cycleTimeNs));
    totalEvasionLatencyNs += static_cast<double>(telem.evasionLatencyNs);
    maxEvasionLatencyNs = std::max(maxEvasionLatencyNs, static_cast<double>(telem.evasionLatencyNs));

    (void)evasionActiveCount;
    (void)deadlineMisses;
    if (telem.evasionActive) ++evasionActiveCount;
    if (telem.deadlineMissed) ++deadlineMisses;

    const auto *agents = swarm.GetAgents();
    for (size_t a = 0; a < swarm.Count(); ++a) {
      float dist = (drone.pos - agents[a].pos).Length();
      if (dist < minObsClearance) minObsClearance = dist;
      if (dist < 0.8f) ++collisionCount;
    }

    int32_t dgx = static_cast<int32_t>(drone.pos.x + 0.5f);
    int32_t dgy = static_cast<int32_t>(drone.pos.y + 0.5f);
    if (!grid.IsWalkable(dgx, dgy)) ++collisionCount;

    if (stepCount >= 5000) break;
  }

  double avgCycleTimeUs = (totalCycleTimeNs / static_cast<double>(stepCount)) / 1000.0;
  double avgEvasionLatencyNs = totalEvasionLatencyNs / static_cast<double>(stepCount);
  double collisionRate = (static_cast<double>(collisionCount) / static_cast<double>(stepCount)) * 100.0;

  printf("  Cycles:%zu | Dist:%.1fm | Evasion:%.1fns | Cycle:%.3fus | Collisions:%zu (%.2f%%)\n",
         stepCount, totalDistanceTraversed, avgEvasionLatencyNs, avgCycleTimeUs, collisionCount, collisionRate);

  assert(collisionCount == 0 && "GATE FAILED: Collision rate must be exactly 0.00%");
  assert(deadlineMisses == 0 && "GATE FAILED: Hard deadline miss rate must be exactly 0.00%");
#if defined(HALO_SANITIZER_ACTIVE)
  assert(avgEvasionLatencyNs < 3500.0 && "GATE FAILED: Sanitized evasion latency too high");
#else
  assert(avgEvasionLatencyNs < 800.0 && "GATE FAILED: Average evasion latency must be strictly < 800 ns");
#endif
  assert(avgCycleTimeUs < 1500.0 && "GATE FAILED: Average cycle time must be < 1.5 ms");

  puts("  ALL EMBEDDED FLIGHT GATES PASSED (0.00% COLLISIONS)");
}

} // namespace halo::test

int main() {
  halo::test::RunDynamicFlightSimulation();
  return 0;
}
