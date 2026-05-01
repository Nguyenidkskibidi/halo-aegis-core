/**
 * ============================================================================
 *  H.A.L.O.* — Post-Processing & Rasterization Pipeline
 * ============================================================================
 *  Convert Theta* macro-waypoints into continuous cell-by-cell micro-paths
 *  compatible with standard navigation controllers (Unity/Unreal/Custom).
 * ============================================================================
 */
#pragma once

// Sửa lại đường dẫn để nó lùi ra ngoài folder navigation rồi tìm vào utils
#include "../utils/halo_types.h" 
#include <vector>
#include <cmath> // Thêm thư mục toán học để dùng std::abs

namespace halo::postprocess {

    /**
     * Kỹ thuật Rasterize bằng Bresenham phi phân nhánh (Branch-optimized).
     * Biến các điểm mốc (waypoints) thưa thớt thành một đường đi liên tục.
     */
    inline std::vector<Vec2i> RasterizePath(const std::vector<Vec2i>& waypoints) noexcept {
        std::vector<Vec2i> fullPath;
        if (waypoints.empty()) return fullPath;

        // Ước lượng dung lượng mảng trước để tránh vector phải resize liên tục (Gây giật lag CPU)
        fullPath.reserve(waypoints.size() * 10); 
        fullPath.push_back(waypoints[0]);

        for (size_t i = 0; i < waypoints.size() - 1; ++i) {
            Vec2i p0 = waypoints[i];
            Vec2i p1 = waypoints[i+1];
            
            int32_t dx = std::abs(p1.x - p0.x);
            int32_t sx = p0.x < p1.x ? 1 : -1;
            int32_t dy = -std::abs(p1.y - p0.y);
            int32_t sy = p0.y < p1.y ? 1 : -1;
            int32_t err = dx + dy, e2;

            while (true) {
                // Kiểm tra xem đã tới điểm đích của đoạn chưa
                if (p0.x == p1.x && p0.y == p1.y) break;
                
                e2 = 2 * err;
                if (e2 >= dy) { err += dy; p0.x += sx; }
                if (e2 <= dx) { err += dx; p0.y += sy; }
                
                // Tránh đẩy trùng điểm cuối vào vector
                if (p0.x != p1.x || p0.y != p1.y) {
                    fullPath.push_back(p0);
                }
            }
            fullPath.push_back(p1);
        }
        return fullPath;
    }

} // namespace halo::postprocess