#include <iostream>
#include <chrono>
#include <iomanip>

#include "halo/core/halo_omnicontext_core.h"

using namespace halo::omnicontext;

struct Vec2i {
    int32_t x, y;
    Vec2i(int32_t _x, int32_t _y) : x(_x), y(_y) {}
};

void RunPureSpeedTest() {
    AdaptiveOmniEngine aegis;
    aegis.Init();

    Vec2i robotPos(2, 15);
    Vec2i victimPos(62, 15);

    // ====================================================================
    // 🔥 15 TẦNG ĐỊA NGỤC (CHỈ GHI ĐÈ 1 LẦN VÀO OMNI-SHADOW)
    // ====================================================================
    for(int y = 0; y < 64; ++y) if (y != 15) aegis.SetBit(0, 30, y);
    for(int x = 0; x < 64; x+=3) aegis.SetBit(1, x, 14);
    for(int x = 20; x < 25; ++x) aegis.SetBit(4, x, 15);
    aegis.SetBit(5, 40, 15);
    for(int y = 10; y < 20; ++y) aegis.SetBit(10, 15, y);
    for(int x = 45; x < 50; ++x) aegis.SetBit(11, x, 15);
    aegis.SetBit(12, 35, 15);
    aegis.SetBit(13, 55, 15);
    for(int x = 50; x < 53; ++x) aegis.SetBit(14, x, 15);

    // ====================================================================
    // 🛡️ BẤM GIỜ TỐC ĐỘ GỐC
    // ====================================================================
    const int iters = 10000000;
    int32_t escapePoint = 0;
    uint64_t checksum = 0;

    std::cout << "🚀 H.A.L.O. AEGIS: Khởi động thuật toán OMNI-SHADOW (Pre-collapsed Map)..." << std::endl;

    auto t1 = std::chrono::high_resolution_clock::now();
    for(int i = 0; i < iters; ++i) {
        escapePoint = aegis.EscapeRaycast(robotPos.x, robotPos.y);
        checksum += escapePoint; 
    }
    auto t2 = std::chrono::high_resolution_clock::now();
    
    // Ép biến này hoạt động để Compiler không xóa sạch vòng lặp
    volatile uint64_t prevent_opt = checksum;

    double totalTimeMs = std::chrono::duration<double, std::milli>(t2 - t1).count();
    double avgNano = (totalTimeMs * 1000000.0) / iters;

    std::cout << "\n================================================================================\n";
    std::cout << " 🌌 H.A.L.O. OMNI-SHADOW: ALGORITHMIC BREAKTHROUGH\n";
    std::cout << "================================================================================\n";
    std::cout << "✅ Tốc độ phá vỡ giới hạn  : " << std::fixed << std::setprecision(10) << avgNano << " ns\n";
    std::cout << "✅ Điểm rủi ro tiếp theo   : (" << escapePoint << ", " << robotPos.y << ")\n";
    std::cout << "🛡️ Checksum an ninh        : " << prevent_opt << "\n";
    std::cout << "================================================================================\n\n";

    for (int y = 12; y < 19; ++y) {
        std::cout << std::setw(2) << y << " ";
        for (int x = 0; x < 64; ++x) {
            if (x == robotPos.x && y == robotPos.y) std::cout << "🤖"; 
            else if (x == victimPos.x && y == victimPos.y) std::cout << "❤️ ";
            else if (x == escapePoint && y == robotPos.y) std::cout << "🎯";
            else {
                bool danger = false; int hitLayer = -1;
                for(int l=0; l<15; ++l) if(aegis.IsBitSet(l, x, y)) { danger = true; hitLayer = l; break; }
                if(danger) {
                    if (hitLayer == 0) std::cout << "██";
                    else if (hitLayer == 10) std::cout << "💥";
                    else if (hitLayer == 11) std::cout << "☁️ ";
                    else if (hitLayer == 12) std::cout << "🔥";
                    else if (hitLayer == 13) std::cout << "🛸";
                    else if (hitLayer == 14) std::cout << "🧲";
                    else if (hitLayer == 5) std::cout << "🦅";
                    else if (hitLayer == 4) std::cout << "⚡";
                    else std::cout << "XX";
                }
                else std::cout << " .";
            }
        }
        std::cout << "\n";
    }
}

int main() {
    RunPureSpeedTest();
    return 0;
}