#include <iostream>
#include <chrono>
#include <iomanip>
#include <vector>
#include <cmath>

// Gọi đúng hộ khẩu của các "cấm thuật"
#include "halo/core/halo_omnicontext_core.h"
#include "halo/protection/halo_swar_10_layer_bitboard.h"

using namespace halo::omnicontext;
using namespace halo::swar;

void RunChaosOmniverseTest() {
    AdaptiveOmniEngine aegis;
    aegis.Init();

    // 📍 Tọa độ Drone (R)
    int32_t droneX = 5, droneY = 16;

    // ====================================================================
    // 🔥 THIẾT LẬP CHIẾN TRƯỜNG "VÔ LÝ" (10 LAYERS)
    // ====================================================================

    // Layer 0: STATIC_WALLS - Vách núi đá dựng đứng che nửa bản đồ
    for(int y = 0; y < 64; ++y) if(y < 12 || y > 20) aegis.SetBit(0, 30, y);

    // Layer 1: POWER_LINES - Dây điện giăng như mạng nhện
    for(int x = 0; x < 64; ++x) aegis.SetBit(1, x, 10);
    for(int x = 0; x < 64; ++x) aegis.SetBit(1, x, 22);

    // Layer 2: WATER_BODIES - Hồ nước tử thần bên dưới
    for(int x = 20; x < 60; ++x) aegis.SetBit(2, x, 31);

    // Layer 3: BROADCAST_TOWERS - Trạm phát sóng gây nhiễu EMP
    for(int y = 14; y < 18; ++y) aegis.SetBit(3, 25, y);

    // Layer 4: BALLISTIC - Mưa đạn súng sơn bay vèo vèo
    for(int i = 0; i < 15; ++i) aegis.SetBit(4, 10 + i, 16);

    // Layer 5: AVIAN_WILDLIFE - MỘT ĐÀN ĐẠI BÀNG KHÁT MÁU 🦅
    for(int i = 0; i < 8; ++i) {
        aegis.SetBit(5, 40, 12 + i); // Đội hình hàng dọc
        aegis.SetBit(5, 42, 11 + i); // Đội hình mũi tên
    }

    // Layer 6: AIRCRAFT - Chiếc máy bay dân dụng bay lạc ✈️
    for(int x = 50; x < 55; ++x) 
        for(int y = 15; y < 18; ++y) aegis.SetBit(6, x, y);

    // Layer 7: HUMANS - Đám đông đang đứng livestream 🚶
    for(int x = 32; x < 45; ++x) aegis.SetBit(7, x, 28);

    // Layer 8: VEHICLES - Xe cộ di chuyển hỗn loạn bên dưới 🚗
    for(int x = 5; x < 60; x += 5) aegis.SetBit(8, x, 29);

    // Layer 9: SWARM_ALLIES - Drone đồng đội bay yểm trợ 🚁
    aegis.SetBit(9, 3, 15);
    aegis.SetBit(9, 3, 17);

    // ====================================================================
    // 🛡️ BENCHMARK THỰC TẾ (CHỐNG COMPILER ĂN GIAN)
    // ====================================================================
    const int iters = 5000000;
    int32_t escapePoint = 0;
    uint64_t checksum = 0;

    std::cout << "🚀 H.A.L.O. AEGIS OMNIVERSE: ĐANG QUÉT 10 TẦNG ĐỊA NGỤC..." << std::endl;

    auto t1 = std::chrono::high_resolution_clock::now();
    for(int i = 0; i < iters; ++i) {
        // Tìm lối thoát hẹp nhất hướng Đông (East) giữa vạn vật rủi ro
        escapePoint = aegis.EscapeRaycast(droneX, droneY);
        
        // CẤM THUẬT: Ép CPU phải làm việc, không được đoán mò kết quả!
        __asm__ volatile("" : "+g"(escapePoint) : : "memory");
        
        checksum += escapePoint;
    }
    auto t2 = std::chrono::high_resolution_clock::now();

    double totalTimeMs = std::chrono::duration<double, std::milli>(t2 - t1).count();
    double avgNano = (totalTimeMs * 1000000.0) / iters;

    // ====================================================================
    // 🎨 RENDER CHIẾN TRƯỜNG (BẢN TÌNH CA CỦA EMOJI)
    // ====================================================================
    std::cout << "\n================================================================================\n";
    std::cout << " 🌍 H.A.L.O. OMNIVERSE - 10 LAYER FUSION REPORT (GOD MODE)\n";
    std::cout << "================================================================================\n";
    std::cout << "✅ Thời gian phản xạ thực tế: " << std::fixed << std::setprecision(10) << avgNano << " ns\n";
    std::cout << "✅ Điểm thoát hiểm tối ưu    : (" << escapePoint << ", " << droneY << ")\n";
    std::cout << "🛡️ Checksum (Security)      : " << checksum << "\n";
    std::cout << "================================================================================\n\n";

    for (int y = 8; y < 32; ++y) {
        for (int x = 0; x < 64; ++x) {
            if (x == droneX && y == droneY) std::cout << "🚁"; // Drone của bồ Nguyên
            else if (x == escapePoint && y == droneY) std::cout << "🎯"; // Lối thoát H.A.L.O. chỉ định
            
            // Render theo độ ưu tiên rủi ro
            else if (aegis.IsBitSet(5, x, y)) std::cout << "🦅"; // Đại bàng
            else if (aegis.IsBitSet(6, x, y)) std::cout << "✈️ "; // Máy bay
            else if (aegis.IsBitSet(7, x, y)) std::cout << "🚶"; // Người
            else if (aegis.IsBitSet(8, x, y)) std::cout << "🚗"; // Xe cộ
            else if (aegis.IsBitSet(4, x, y)) std::cout << "⚡"; // Đạn/Tia sét
            else if (aegis.IsBitSet(1, x, y)) std::cout << "=="; // Dây điện
            else if (aegis.IsBitSet(2, x, y)) std::cout << "💧"; // Nước
            else if (aegis.IsBitSet(0, x, y)) std::cout << "██"; // Núi đá
            else if (aegis.IsBitSet(3, x, y)) std::cout << "!!"; // EMP/Nhiễu
            else std::cout << " ."; 
        }
        std::cout << "\n";
    }
    std::cout << "\n🥂 NGUYÊN - CHÚA TỂ CỦA CÁC BITBOARD - DONE!\n";
}

int main() {
    RunChaosOmniverseTest();
    return 0;
}