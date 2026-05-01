#include <iostream>
#include <chrono>
#include <iomanip>
#include <vector>

// Đảm bảo bồ đã sửa đường dẫn include như anh em mình bàn nhé!
#include "halo/core/halo_omnicontext_core.h"
#include "halo/protection/halo_swar_10_layer_bitboard.h"

using namespace halo::omnicontext;
using namespace halo::swar;

struct Vec2i {
    int32_t x, y;
    Vec2i(int32_t _x, int32_t _y) : x(_x), y(_y) {}
};

void RunLongRangeExtremeTest() {
    AdaptiveOmniEngine aegis;
    aegis.Init();

    // 📍 ĐIỂM BẮT ĐẦU VÀ KẾT THÚC CỰC XA
    Vec2i robotPos(2, 15);      // Robot xuất phát bên trái
    Vec2i victimPos(62, 15);    // Nạn nhân ở tận cùng bên phải trên cùng một dòng y

    // ====================================================================
    // 🔥 THIẾT LẬP CHIẾN TRƯỜNG PHỨC TẠP (10 LAYERS)
    // ====================================================================

    // Layer 0: Vách đá chắn đường ở giữa
    for(int y = 0; y < 64; ++y) {
        if (y != 15) aegis.SetBit(0, 31, y); // Để hở duy nhất một khe hẹp ở y=15
    }

    // Layer 1: Dây điện chằng chịt
    for(int x = 0; x < 64; x += 2) aegis.SetBit(1, x, 14);
    for(int x = 1; x < 64; x += 2) aegis.SetBit(1, x, 16);

    // Layer 5: ĐÀN ĐẠI BÀNG KHÁT MÁU 🦅 (Chặn đường ở x=45)
    for(int y = 10; y < 20; ++y) {
        if (y % 2 == 0) aegis.SetBit(5, 45, y); 
    }

    // Layer 4: Đạn bay lạc ⚡ (Gần vị trí nạn nhân)
    for(int x = 55; x < 60; ++x) aegis.SetBit(4, x, 15);

    // Layer 7: Đám đông cứu hộ bên dưới 🚶
    for(int x = 10; x < 60; x += 5) aegis.SetBit(7, x, 18);

    // ====================================================================
    // 🛡️ BENCHMARK THỰC TẾ (10 TRIỆU LẦN QUÉT TỪ XA)
    // ====================================================================
    const int iters = 10000000;
    int32_t escapePoint = 0;
    uint64_t checksum = 0;

    std::cout << "🚀 H.A.L.O. AEGIS: Đang quét lộ trình chiến thuật tầm xa..." << std::endl;

    auto t1 = std::chrono::high_resolution_clock::now();
    for(int i = 0; i < iters; ++i) {
        // Quét từ Robot (x=2) đến tận cùng để tìm vật cản đầu tiên trên line 15
        escapePoint = aegis.EscapeRaycast(robotPos.x, robotPos.y);
        
        // CẤM THUẬT: Chống Compiler tối ưu bỏ vòng lặp
        __asm__ volatile("" : "+g"(escapePoint) : : "memory");
        
        checksum += escapePoint;
    }
    auto t2 = std::chrono::high_resolution_clock::now();

    double totalTimeMs = std::chrono::duration<double, std::milli>(t2 - t1).count();
    double avgNano = (totalTimeMs * 1000000.0) / iters;

    // ====================================================================
    // 🎨 RENDER "BẢN ĐỒ SINH TỒN"
    // ====================================================================
    std::cout << "\n================================================================================\n";
    std::cout << " 🌍 H.A.L.O. LONG-RANGE TACTICAL REPORT\n";
    std::cout << "================================================================================\n";
    std::cout << "✅ Tốc độ phản xạ trung bình : " << std::fixed << std::setprecision(10) << avgNano << " ns\n";
    std::cout << "✅ Điểm va chạm/thoát hiểm   : (" << escapePoint << ", " << robotPos.y << ")\n";
    std::cout << "🛡️ Checksum hệ thống        : " << checksum << "\n";
    std::cout << "================================================================================\n\n";

    for (int y = 10; y < 22; ++y) {
        std::cout << std::setw(2) << y << " ";
        for (int x = 0; x < 64; ++x) {
            if (x == robotPos.x && y == robotPos.y) std::cout << "🤖"; 
            else if (x == victimPos.x && y == victimPos.y) std::cout << "❤️ ";
            else if (x == escapePoint && y == robotPos.y) std::cout << "🎯";
            else {
                bool danger = false;
                for(int l=0; l<10; ++l) if(aegis.IsBitSet(l, x, y)) danger = true;
                if(danger) {
                    if (aegis.IsBitSet(5, x, y)) std::cout << "🦅";
                    else if (aegis.IsBitSet(0, x, y)) std::cout << "██";
                    else if (aegis.IsBitSet(1, x, y)) std::cout << "==";
                    else if (aegis.IsBitSet(4, x, y)) std::cout << "⚡";
                    else std::cout << "XX";
                }
                else std::cout << " .";
            }
        }
        std::cout << "\n";
    }
    std::cout << "\nCảm ơn mấy anh chị đã chạy nhen!\n";
}

int main() {
    RunLongRangeExtremeTest();
    return 0;
}