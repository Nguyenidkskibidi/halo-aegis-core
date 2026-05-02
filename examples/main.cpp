#include <chrono>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

#include "/Users/nguyenmanhhung/halo-aegis-core/include/halo/core/halo_omnicontext_core.h"

using namespace halo::omnicontext;

struct Vec2i {
  int32_t x, y;
  Vec2i(int32_t _x, int32_t _y) : x(_x), y(_y) {}
};

struct Localization {
  std::string startMsg;
  std::string header;
  std::string speedLabel;
  std::string riskLabel;
  std::string checksumLabel;
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
  int choice;
  std::cout << "==========================================\n";
  std::cout << "   SELECT LANGUAGE / CHỌN NGÔN NGỮ\n";
  std::cout << "   1. Tiếng Việt (Vietnam Mode 😎)\n";
  std::cout << "   2. English (International Mode 🌍)\n";
  std::cout << "==========================================\n";
  std::cout << "Choice/Lựa chọn (1-2): ";
  std::cin >> choice;

  Localization lang;
  if (choice == 1) {
    lang = {"🚀 H.A.L.O. AEGIS: KHỞI ĐỘNG MA TRẬN ĐỊA NGỤC (96 TỶ LỆNH)...",
            "🌌 H.A.L.O. OMNI-SHADOW: PHÂN TÍCH ĐƯỜNG ĐI LƯỢNG TỬ",
            "✅ Tốc độ VẬT LÝ phá vỡ giới hạn : ",
            "✅ Điểm rủi ro (Raycast đâm trúng) : ",
            "🛡️ Checksum an ninh              : "};
  } else {
    lang = {"🚀 H.A.L.O. AEGIS: HELL MATRIX ACTIVATED (96 BILLION OPS)...",
            "🌌 H.A.L.O. OMNI-SHADOW: QUANTUM PATH ANALYSIS",
            "✅ PHYSICAL Breaking Speed Limit : ",
            "✅ Raycast Impact Vector         : ",
            "🛡️ Security Checksum             : "};
  }

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

  std::cout << "\n" << lang.startMsg << std::endl;

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

  std::cout << "\n================================================================================\n";
  std::cout << " " << lang.header << "\n";
  std::cout << "================================================================================\n";

  std::cout << lang.speedLabel << std::fixed << std::setprecision(11) << avgMs << " ms\n";
  std::cout << lang.riskLabel << "(" << escapePoint << ", " << robotPos.y << ")\n";
  std::cout << lang.checksumLabel << prevent_opt << "\n";
  std::cout << "================================================================================\n\n";

  for (int y = 10; y <= 20; ++y) {
    std::cout << std::setw(2) << y << " ";
    for (int x = 0; x < 64; ++x) {
      if (x == robotPos.x && y == robotPos.y)
        std::cout << "🤖";
      else if (x == victimPos.x && y == victimPos.y)
        std::cout << "❤️ ";
      else if (IsSafePath(x, y))
        std::cout << "✨";
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
          if (hitLayer == 0) std::cout << "██";
          else if (hitLayer == 10) std::cout << "💥";
          else if (hitLayer == 11) std::cout << "☁️ ";
          else if (hitLayer == 12) std::cout << "🔥";
          else if (hitLayer == 13) std::cout << "🛸";
          else if (hitLayer == 14) std::cout << "🧲";
          else if (hitLayer == 5) std::cout << "🦅";
          else if (hitLayer == 4) std::cout << "⚡";
          else std::cout << "XX";
        } else {
          std::cout << " .";
        }
      }
    }
    std::cout << "\n";
  }
}

int main() {
  RunTrueHardwareTest();
  return 0;
}