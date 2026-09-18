#include <chrono>
#include <cstdint>
#include <cstdio>

#include "halo/core/halo_omnicontext_core.h"

using namespace halo::omnicontext;

struct Vec2i {
  int32_t x, y;
  Vec2i(int32_t _x, int32_t _y) : x(_x), y(_y) {}
};

struct Localization {
  const char *startMsg;
  const char *header;
  const char *speedLabel;
  const char *riskLabel;
  const char *checksumLabel;
};

#define RAYCAST_1 checksum += aegis.EscapeRaycast(robotPos.x, robotPos.y)
#define RAYCAST_4 RAYCAST_1; RAYCAST_1; RAYCAST_1; RAYCAST_1
#define RAYCAST_16 RAYCAST_4; RAYCAST_4; RAYCAST_4; RAYCAST_4
#define RAYCAST_64 RAYCAST_16; RAYCAST_16; RAYCAST_16; RAYCAST_16

bool IsSafePath(int x, int y) {
  if (x >= 1 && x <= 8 && y == 15) return true;
  if (x == 8 && y >= 12 && y <= 15) return true;
  if (x >= 8 && x <= 22 && y == 12) return true;
  if (x == 22 && y >= 12 && y <= 18) return true;
  if (x >= 22 && x <= 38 && y == 18) return true;
  if (x == 38 && y >= 14 && y <= 18) return true;
  if (x >= 38 && x <= 50 && y == 14) return true;
  if (x == 50 && y >= 14 && y <= 17) return true;
  if (x >= 50 && x <= 62 && y == 17) return true;
  return false;
}

void RunTrueHardwareTest() {
  Localization lang = {"[HALO] HELL MATRIX ACTIVATED (96 BILLION OPS)...",
                       "HALO OMNI-SHADOW: QUANTUM PATH ANALYSIS",
                       "PHYSICAL Breaking Speed Limit : ",
                       "Raycast Impact Vector         : ",
                       "Security Checksum             : "};

  AdaptiveOmniEngine aegis;
  aegis.Init();

  Vec2i robotPos(1, 15);
  Vec2i victimPos(62, 17);

  for (int y = 10; y <= 20; ++y) {
    for (int x = 0; x < 64; ++x) {
      if (IsSafePath(x, y)) continue;
      if (x == robotPos.x && y == robotPos.y) continue;
      if (x == victimPos.x && y == victimPos.y) continue;

      if (x % 7 == 0) aegis.SetBit(0, x, y);
      else if (x % 11 == 0) aegis.SetBit(10, x, y);
      else if (x % 13 == 0) aegis.SetBit(12, x, y);
      else if (y == 13 && x > 10 && x < 60) aegis.SetBit(4, x, y);
      else if (y == 16 && x > 5 && x < 55) aegis.SetBit(14, x, y);
      else if ((x + y) % 9 == 0) aegis.SetBit(13, x, y);
      else if ((x * y) % 17 == 0) aegis.SetBit(5, x, y);
    }
  }

  const uint64_t BATCHES = 1500000000ULL;
  const uint64_t TOTAL_ITERS = BATCHES * 64;

  uint64_t checksum = 0;

  std::printf("\n%s\n", lang.startMsg);

  auto t1 = std::chrono::high_resolution_clock::now();

  for (uint64_t i = 0; i < BATCHES; ++i) {
    RAYCAST_64;
  }

  auto t2 = std::chrono::high_resolution_clock::now();

  volatile uint64_t prevent_opt = checksum;
  int32_t escapePoint = aegis.EscapeRaycast(robotPos.x, robotPos.y);

  long double totalTimeMs =
      std::chrono::duration<long double, std::milli>(t2 - t1).count();
  long double avgMs = totalTimeMs / static_cast<long double>(TOTAL_ITERS);

  std::printf("\n================================================================================\n");
  std::printf(" %s\n", lang.header);
  std::printf("================================================================================\n");

  std::printf("%s%.11Lf ms\n", lang.speedLabel, avgMs);
  std::printf("%s(%d, %d)\n", lang.riskLabel, escapePoint, robotPos.y);
  std::printf("%s%llu\n", lang.checksumLabel, (unsigned long long)prevent_opt);
  std::printf("================================================================================\n\n");

  for (int y = 10; y <= 20; ++y) {
    std::printf("%2d ", y);
    for (int x = 0; x < 64; ++x) {
      if (x == robotPos.x && y == robotPos.y)
        std::printf("🤖");
      else if (x == victimPos.x && y == victimPos.y)
        std::printf("❤️ ");
      else if (IsSafePath(x, y))
        std::printf("✨");
      else {
        bool danger = false;
        int hitLayer = -1;
        for (int l = 0; l < 15; ++l)
          if (aegis.IsBitSet(l, x, y)) {
            danger = true;
            hitLayer = l;
            break;
          }
        if (danger) {
          if (hitLayer == 0) std::printf("██");
          else if (hitLayer == 10) std::printf("💥");
          else if (hitLayer == 11) std::printf("☁️ ");
          else if (hitLayer == 12) std::printf("🔥");
          else if (hitLayer == 13) std::printf("🛸");
          else if (hitLayer == 14) std::printf("🧲");
          else if (hitLayer == 5) std::printf("🦅");
          else if (hitLayer == 4) std::printf("⚡");
          else std::printf("XX");
        } else {
          std::printf(" .");
        }
      }
    }
    std::printf("\n");
  }
}

int main() {
  RunTrueHardwareTest();
  return 0;
}