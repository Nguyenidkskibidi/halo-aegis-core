/*
 * H.A.L.O. Aegis Core - ESP32 Plug & Play Autonomous Navigation Demo
 *
 * Designed for ESP32 / ESP32-S3 / ESP32-C3
 * Features: Zero-Heap Static Allocation, Sub-Microsecond Reflex, FreeRTOS Ready
 */

#include <Arduino.h>
#include <halo/core/halo_supreme_core.h>

using namespace halo;
using namespace halo::core;

// 1. Static Memory Pool (Allocated in BSS - Zero Dynamic Heap Allocation)
// 32x32 navigation requires ~57 KB of RAM, easily fitting inside ESP32 internal SRAM (320 KB)
alignas(64) static uint8_t s_navPool[64 * 1024];
alignas(64) static uint8_t s_walkable[32 * 32];
alignas(64) static int32_t s_penalties[32 * 32];

// 2. Instantiate Engine & Grid
static GridT<32, 32> s_grid;
static EmbeddedSupremeEngine32 s_engine;

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("\n=======================================================");
  Serial.println("  H.A.L.O. Aegis Core - ESP32 Embedded Autonomous Nav  ");
  Serial.println("=======================================================");

  // Initialize 32x32 Grid
  s_grid.Init(32, 32, s_walkable, s_penalties);

  // Inject a simulated canyon obstacle wall with a central breach
  for (int y = 5; y < 27; ++y) {
    s_grid.SetWalkable(16, y, false);
  }
  s_grid.SetWalkable(16, 16, true); // Passage gate

  // Boot Engine with Static BSS Buffer (Zero malloc/free calls)
  s_engine.BootSystemWithBuffer(&s_grid, s_navPool, sizeof(s_navPool));

  Serial.printf("Static Pool Initialized: %u KB\n", (unsigned)(sizeof(s_navPool) / 1024));
  Serial.printf("Arena Memory Consumed  : %u Bytes\n", (unsigned)s_engine.GetMasterArenaOffset());
  Serial.printf("Arena Memory Remaining : %u Bytes\n", (unsigned)s_engine.GetMasterArenaCapacity() - (unsigned)s_engine.GetMasterArenaOffset());

  // Test Run: Plan path across obstacle wall
  Vec2i start{4, 16};
  Vec2i target{28, 16};

  uint32_t t0 = micros();
  PathResult result = s_engine.RouteGridOptimal(start, target);
  uint32_t t1 = micros();

  if (result.found) {
    Serial.printf("\n✅ Path Solved in %u microseconds! (Waypoints: %d)\n", (unsigned)(t1 - t0), result.len);
    Serial.println("Route Coordinates:");
    for (int i = 0; i < result.len; ++i) {
      Serial.printf("  [%02d] -> (%d, %d)\n", i, result.route[i].x, result.route[i].y);
    }

    // Any-Angle Shortening Test
    ContinuousPathResult smoothResult = s_engine.RouteGridAnyAngle(start, target);
    Serial.printf("Any-Angle String Pulling: %d pruned waypoints (Euclidean shortest)\n", smoothResult.len);
  } else {
    Serial.println("❌ Pathfinding failed!");
  }
}

void loop() {
  // Real-time obstacle avoidance loop (e.g. drone lidar / ultrasonic sensor update)
  delay(2000);

  Vec2i dynamicStart{random(1, 10), random(1, 30)};
  Vec2i dynamicTarget{random(20, 30), random(1, 30)};

  uint32_t t0 = micros();
  PathResult res = s_engine.RouteGrid(dynamicStart, dynamicTarget);
  uint32_t elapsedUs = micros() - t0;

  if (res.found) {
    Serial.printf("[UAV Reflex] From (%d,%d) to (%d,%d): %d waypoints in %u µs\n",
                  dynamicStart.x, dynamicStart.y, dynamicTarget.x, dynamicTarget.y,
                  res.len, (unsigned)elapsedUs);
  }
}
