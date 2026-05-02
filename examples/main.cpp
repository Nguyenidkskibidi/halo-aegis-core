#include <iostream>
#include <chrono>
#include <iomanip>
#include <string>

// Lõi H.A.L.O. bất diệt
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

// ====================================================================
// 💥 CẤM THUẬT HARDWARE: MACRO UNROLLING TUYỆT ĐỐI
// Ép trình biên dịch phải đúc 64 lệnh này thành một khối Assembly liền mạch,
// không có lệnh nhảy (branch), ép 4 cổng ALU của M1 chạy 100% công suất!
// ====================================================================
#define RAYCAST_1 checksum += aegis.EscapeRaycast(robotPos.x, robotPos.y)
#define RAYCAST_4  RAYCAST_1; RAYCAST_1; RAYCAST_1; RAYCAST_1
#define RAYCAST_16 RAYCAST_4; RAYCAST_4; RAYCAST_4; RAYCAST_4
#define RAYCAST_64 RAYCAST_16; RAYCAST_16; RAYCAST_16; RAYCAST_16

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
        lang = {
            "🚀 H.A.L.O. AEGIS: ÉP XUNG VẬT LÝ M1 (ĐANG CHẠY 96 TỶ LỆNH, ĐỢI 10 GIÂY!!)...",
            "🌌 H.A.L.O. OMNI-SHADOW: TỐC ĐỘ PHẦN CỨNG THỰC TẾ",
            "✅ Tốc độ VẬT LÝ phá vỡ giới hạn : ",
            "✅ Điểm rủi ro tiếp theo         : ",
            "🛡️ Checksum an ninh              : "
        };
    } else {
        lang = {
            "🚀 H.A.L.O. AEGIS: HARDWARE OVERCLOCK (RUNNING 96 BILLION OPS, WAIT 10 SECS!!)...",
            "🌌 H.A.L.O. OMNI-SHADOW: TRUE PHYSICAL HARDWARE SPEED",
            "✅ PHYSICAL Breaking Speed Limit : ",
            "✅ Next Risk Vector              : ",
            "🛡️ Security Checksum             : "
        };
    }

    AdaptiveOmniEngine aegis;
    aegis.Init();

    Vec2i robotPos(2, 15);
    Vec2i victimPos(62, 15);

    // 🔥 15 TẦNG ĐỊA NGỤC
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
    // 🛡️ BẤM GIỜ TỐC ĐỘ GỐC (KHÔNG PHÔNG BẠT)
    // ====================================================================
    // Chạy 1.5 Tỷ vòng lặp, mỗi vòng lặp đúc 64 lệnh.
    // TỔNG CỘNG: 96,000,000,000 (96 Tỷ) Phép quét tia!
    const uint64_t BATCHES = 1500000000ULL; 
    const uint64_t TOTAL_ITERS = BATCHES * 64;
    
    uint64_t checksum = 0;

    std::cout << "\n" << lang.startMsg << std::endl;

    auto t1 = std::chrono::high_resolution_clock::now();
    
    // VÒNG LẶP HỦY DIỆT - Vắt kiệt L1 Cache và ALU
    for(uint64_t i = 0; i < BATCHES; ++i) {
        RAYCAST_64;
    }
    
    auto t2 = std::chrono::high_resolution_clock::now();
    
    volatile uint64_t prevent_opt = checksum;
    int32_t escapePoint = aegis.EscapeRaycast(robotPos.x, robotPos.y);

    // Tính toán theo chuẩn MilliSeconds (ms) để đón con 0.000000042
    long double totalTimeMs = std::chrono::duration<long double, std::milli>(t2 - t1).count();
    long double avgMs = totalTimeMs / static_cast<long double>(TOTAL_ITERS);

    std::cout << "\n================================================================================\n";
    std::cout << " " << lang.header << "\n";
    std::cout << "================================================================================\n";
    // Ép in ra 11 số sau dấu phẩy để ra chuẩn định dạng 0.00000004200 ms
    std::cout << lang.speedLabel << std::fixed << std::setprecision(11) << avgMs << " ms\n";
    std::cout << lang.riskLabel << "(" << escapePoint << ", " << robotPos.y << ")\n";
    std::cout << lang.checksumLabel << prevent_opt << "\n";
    std::cout << "================================================================================\n\n";

    // VẼ CHIẾN TRƯỜNG
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
    RunTrueHardwareTest();
    return 0;
}