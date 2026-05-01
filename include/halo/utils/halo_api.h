/**
 * ============================================================================
 *  H.A.L.O.* — Universal C-API Wrapper (Foreign Function Interface)
 * ============================================================================
 *  Dùng để nhúng lõi H.A.L.O.* vào C#, Python, Rust, Go, JS/WebAssembly, v.v.
 * ============================================================================
 */
#pragma once

#include <stdint.h>
#include <stdbool.h>

// Macro định nghĩa xuất file DLL / Shared Object
#if defined(_WIN32) || defined(_WIN64)
    #define HALO_EXPORT __declspec(dllexport)
#else
    #define HALO_EXPORT __attribute__((visibility("default")))
#endif

extern "C" {

    // --- STRUCT TRUNG GIAN GIAO TIẾP VỚI CÁC NGÔN NGỮ KHÁC ---
    struct HaloRouteResult {
        bool found;
        int32_t pathLength;
        int32_t waypointsX[1024]; // Mảng trả tọa độ X
        int32_t waypointsY[1024]; // Mảng trả tọa độ Y
        float totalCost;
    };

    // --- API QUẢN LÝ VÒNG ĐỜI (Lò phản ứng) ---
    // Khởi tạo toàn bộ hệ thống bằng 1 hàm duy nhất
    HALO_EXPORT void* Halo_InitSystem(int32_t memoryMB);
    
    // Tắt hệ thống, trả RAM lại cho OS
    HALO_EXPORT void Halo_ShutdownSystem(void* systemPtr);


    // --- API CHO LƯỚI 2D (GAME MAZE / MINECRAFT) ---
    HALO_EXPORT void Halo_Grid_InitMap(void* systemPtr, int32_t width, int32_t height);
    HALO_EXPORT void Halo_Grid_SetObstacle(void* systemPtr, int32_t x, int32_t y);
    HALO_EXPORT void Halo_Grid_BakeClearance(void* systemPtr); // Pre-compute
    
    // Hàm tìm đường mà C# / Python sẽ gọi!
    HALO_EXPORT HaloRouteResult Halo_Grid_FindPath(void* systemPtr, int32_t startX, int32_t startY, int32_t targetX, int32_t targetY);


    // --- API CHO ĐÔ THỊ GRAPH (GOOGLE MAPS / GTA) ---
    HALO_EXPORT void Halo_Urban_InitCity(void* systemPtr, int32_t maxNodes, int32_t maxEdges);
    HALO_EXPORT int32_t Halo_Urban_AddIntersection(void* systemPtr, float x, float y);
    HALO_EXPORT void Halo_Urban_AddRoad(void* systemPtr, int32_t fromId, int32_t toId, float trafficJam);
    
    // Hàm chỉ đường chống kẹt xe!
    HALO_EXPORT HaloRouteResult Halo_Urban_RouteTraffic(void* systemPtr, int32_t startId, int32_t targetId);

} // extern "C"