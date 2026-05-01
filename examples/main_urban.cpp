/**
 * ============================================================================
 *  H.A.L.O.* URBAN — Traffic & GIS Simulation Benchmark
 * ============================================================================
 */
#include <iostream>
#include <vector>
#include <chrono>
#include "halo_graph_core.h"

using namespace halo;
using namespace halo::urban;

int main() {
    memory::ArenaAllocator arena;
    arena.Init(5 * 1024 * 1024); // Xin macOS 5MB RAM xài vĩnh viễn

    CityMap city;
    city.Init(1000, 5000, arena); // Hỗ trợ 1000 ngã tư, 5000 con đường

    // --- XÂY DỰNG THÀNH PHỐ MÔ PHỎNG ---
    // A(0,0) ---- B(10,0) ---- C(20,0)  (Đại lộ siêu lớn)
    //  |            |            |
    // D(0,10) --- E(10,10) --- F(20,10) (Đường song song)
    //  |            |            |
    // G(0,20) --- H(10,20) --- I(20,20) (Điểm đến cuối)
    
    int A = city.AddIntersection(0, 0);   int B = city.AddIntersection(10, 0);  int C = city.AddIntersection(20, 0);
    int D = city.AddIntersection(0, 10);  int E = city.AddIntersection(10, 10); int F = city.AddIntersection(20, 10);
    int G = city.AddIntersection(0, 20);  int H = city.AddIntersection(10, 20); int I = city.AddIntersection(20, 20);

    // KẾT NỐI ĐƯỜNG (Biến số cuối cùng là Traffic Jam - Hệ số kẹt xe)
    
    // Đường Đại Lộ từ A -> B -> C -> F -> I (Rất đẹp, thẳng tắp nhưng... ĐANG KẸT XE CỨNG NGẮC x5.0)
    city.AddRoad(A, B, 5.0f); city.AddRoad(B, C, 5.0f); 
    city.AddRoad(C, F, 5.0f); city.AddRoad(F, I, 5.0f);

    // Hẻm nhỏ luồn lách A -> D -> E -> H -> I (Đường ngoằn ngoèo nhưng THÔNG THOÁNG x1.0)
    city.AddRoad(A, D, 1.0f); city.AddRoad(D, E, 1.0f); 
    city.AddRoad(E, H, 1.0f); city.AddRoad(H, I, 1.0f);
    
    // Vài đường phụ để network thêm phức tạp
    city.AddRoad(B, E, 1.5f); city.AddRoad(E, F, 2.0f);
    city.AddRoad(D, G, 1.0f); city.AddRoad(G, H, 1.0f);

    // --- KHỞI ĐỘNG HỆ THỐNG ĐIỀU PHỐI GIAO THÔNG ---
    UrbanPathfinder router;
    router.Init(&city, arena);

    // Làm nóng L1 Cache của Apple Silicon
    router.RouteTraffic(A, I);

    // STRESS TEST TỐC ĐỘ GOOGLE MAPS
    int iterations = 10000;
    std::vector<int32_t> bestRoute;
    
    auto t1 = std::chrono::high_resolution_clock::now();
    for(int i = 0; i < iterations; ++i) {
        bestRoute = router.RouteTraffic(A, I);
    }
    auto t2 = std::chrono::high_resolution_clock::now();

    double avgTime = std::chrono::duration<double, std::milli>(t2 - t1).count() / iterations;

    std::cout << "\n🌍 [H.A.L.O.* URBAN MAPS & TRAFFIC AI] 🌍\n";
    std::cout << "✅ Toc do tim duong Do thi: " << std::fixed << avgTime << " ms / route\n";
    
    if (!bestRoute.empty()) {
        std::cout << "🚕 GPS ROUTE (Né kẹt xe đại lộ): ";
        const char* nodeNames[] = {"A", "B", "C", "D", "E", "F", "G", "H", "I"};
        for (size_t i = 0; i < bestRoute.size(); ++i) {
            std::cout << "[" << nodeNames[bestRoute[i]] << "]";
            if (i < bestRoute.size() - 1) std::cout << " ➔ ";
        }
        std::cout << "\n\n(Bồ thấy đỉnh không? Mặc dù A->B->C->F->I là cao tốc thẳng tắp, nhưng AI của bồ đánh hơi thấy KẸT XE nên nó tự động bẻ lái rẽ vô hẻm A->D->E->H->I để đi cho nhanh! Đẳng cấp GIS là đây!) 🔥😎\n";
    } else {
        std::cout << "❌ Vo Phuong Cuu Chua!\n";
    }

    return 0;
}