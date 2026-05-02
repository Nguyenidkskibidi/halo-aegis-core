#include <iostream>
#include <chrono>
#include <iomanip>
#include <string>

// Giữ nguyên file core thần thánh của bồ
#include "halo/core/halo_omnicontext_core.h"

using namespace halo::omnicontext;

struct Vec2i {
    int32_t x, y;
    Vec2i(int32_t _x, int32_t _y) : x(_x), y(_y) {}
};

// Cấu trúc để quản lý ngôn ngữ cho chuyên nghiệp
struct Localization {
    std::string startMsg;
    std::string header;
    std::string speedLabel;
    std::string riskLabel;
    std::string checksumLabel;
};

void RunPureSpeedTest() {
    // --- BƯỚC 1: CHỌN NGÔN NGỮ ---
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
        lang = {
            "🚀 H.A.L.O. AEGIS: Khởi động thuật toán OMNI-SHADOW (Bản đồ nén sẵn)...",
            "🌌 H.A.L.O. OMNI-SHADOW: ĐỘT PHÁ THUẬT TOÁN",
            "✅ Tốc độ phá vỡ giới hạn  : ",
            "✅ Điểm rủi ro tiếp theo   : ",
            "🛡️ Checksum an ninh        : "
        };
    } else {
        lang = {
            "🚀 H.A.L.O. AEGIS: Deploying OMNI-SHADOW (Pre-collapsed Map)...",
            "🌌 H.A.L.O. OMNI-SHADOW: ALGORITHMIC BREAKTHROUGH",
            "✅ Breaking Speed Limit    : ",
            "✅ Next Risk Vector        : ",
            "🛡️ Security Checksum       : "
        };
    }

    AdaptiveOmniEngine aegis;
    aegis.Init();

    Vec2i robotPos(2, 15);
    Vec2i victimPos(62, 15);

    // ====================================================================
    // 🔥 15 TẦNG ĐỊA NGỤC (Giữ nguyên logic của bồ)
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

    std::cout << "\n" << lang.startMsg << std::endl;

    auto t1 = std::chrono::high_resolution_clock::now();
    for(int i = 0; i < iters; ++i) {
        escapePoint = aegis.EscapeRaycast(robotPos.x, robotPos.y);
        checksum += (uint32_t)escapePoint; 
    }
    auto t2 = std::chrono::high_resolution_clock::now();
    
    volatile uint64_t prevent_opt = checksum;

    double totalTimeMs = std::chrono::duration<double, std::milli>(t2 - t1).count();
    double avgNano = (totalTimeMs * 1000000.0) / iters;

    std::cout << "\n================================================================================\n";
    std::cout << " " << lang.header << "\n";
    std::cout << "================================================================================\n";
    std::cout << lang.speedLabel << std::fixed << std::setprecision(10) << avgNano << " ns\n";
    std::cout << lang.riskLabel << "(" << escapePoint << ", " << robotPos.y << ")\n";
    std::cout << lang.checksumLabel << prevent_opt << "\n";
    std::cout << "================================================================================\n\n";

    // VẼ BẢN ĐỒ (Giữ nguyên emoji vì nó là ngôn ngữ quốc tế)
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
    
    if (choice == 1) std::cout << "\n[HỆ THỐNG]: Sẵn sàng cứu hộ! (Thủ Đức over and out!)\n";
    else std::cout << "\n[SYSTEM]: Rescue Ready! (H.A.L.O. over and out!)\n";
}

int main() {
    RunPureSpeedTest();
    return 0;
}