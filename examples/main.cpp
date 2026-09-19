#include <chrono>
#include <cstdint>
#include <cstdio>

#include "halo/core/halo_memory.h"
#include "halo/core/halo_omnicontext_core.h"

using namespace halo::omnicontext;

template <typename T>
[[gnu::always_inline]] inline void DoNotOptimize(T const& val) {
  asm volatile("" : : "g"(val) : "memory");
}

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
  Localization lang = {"[HALO] MULTI-LAYER REFLEX MATRIX INITIALIZED (10,000,000 REAL RAYCASTS)...",
                       "HALO OMNI-SHADOW: QUANTUM PATH ANALYSIS",
                       "Measured Hardware Latency    : ",
                       "Raycast Impact Vector         : ",
                       "Security Checksum (Sinked)    : "};

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

  constexpr uint64_t TOTAL_ITERS = 10000000ULL;
  constexpr uint64_t RAYS_PER_ROW = 32;
  constexpr uint64_t NUM_ROWS = TOTAL_ITERS / RAYS_PER_ROW; // 312,500 rows

  uint64_t checksum = 0;

  std::printf("\n%s\n", lang.startMsg);

  // Warmup
  for (uint64_t i = 0; i < 50000; ++i) {
    checksum += aegis.EscapeRaycast(static_cast<int32_t>(i & 31), static_cast<int32_t>((i * 3) & 63));
  }
  DoNotOptimize(checksum);
  checksum = 0;

  auto t1 = std::chrono::steady_clock::now();

  for (uint64_t i = 0; i < NUM_ROWS; ++i) {
    int32_t y = static_cast<int32_t>((i * 7) & 63);
    uint64_t row = aegis.GetShadowRow(y);

    #define R(offset) checksum += AdaptiveOmniEngine::RaycastRow(row, offset)
    R(0);  R(1);  R(2);  R(3);  R(4);  R(5);  R(6);  R(7);
    R(8);  R(9);  R(10); R(11); R(12); R(13); R(14); R(15);
    R(16); R(17); R(18); R(19); R(20); R(21); R(22); R(23);
    R(24); R(25); R(26); R(27); R(28); R(29); R(30); R(31);
    #undef R
  }

  auto t2 = std::chrono::steady_clock::now();
  DoNotOptimize(checksum);

  int32_t escapePoint = aegis.EscapeRaycast(robotPos.x, robotPos.y);

  double totalNs = std::chrono::duration_cast<std::chrono::nanoseconds>(t2 - t1).count();
  double avgNs = totalNs / static_cast<double>(TOTAL_ITERS);

  std::printf("\n================================================================================\n");
  std::printf(" %s\n", lang.header);
  std::printf("================================================================================\n");

  std::printf("%s%.4f ns / op (%.3f ms total)\n", lang.speedLabel, avgNs, totalNs / 1e6);
  std::printf("%s(%d, %d)\n", lang.riskLabel, escapePoint, robotPos.y);
  std::printf("%s%llu\n", lang.checksumLabel, (unsigned long long)checksum);
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
        if (aegis.IsBitSet(10, x, y)) {
          std::printf("💥");
          danger = true;
        } else if (aegis.IsBitSet(12, x, y)) {
          std::printf("🔥");
          danger = true;
        } else if (aegis.IsBitSet(13, x, y)) {
          std::printf("🛸");
          danger = true;
        } else if (aegis.IsBitSet(14, x, y)) {
          std::printf("🧲");
          danger = true;
        } else if (aegis.IsBitSet(4, x, y)) {
          std::printf("⚡");
          danger = true;
        } else if (aegis.IsBitSet(5, x, y)) {
          std::printf("🦅");
          danger = true;
        } else if (aegis.IsBitSet(0, x, y)) {
          std::printf("██");
          danger = true;
        }
        if (!danger) std::printf(" .");
      }
    }
    std::printf("\n");
  }
}

int main() {
  halo::memory::PinThreadToPerformanceCore(0);
  RunTrueHardwareTest();
  return 0;
}