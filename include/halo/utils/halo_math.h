#pragma once
#include "halo_types.h"
#include <cstdlib>

namespace halo::math {
    // THUẬT TOÁN CĂNG DÂY VECTOR TIE-BREAKER
    inline int32_t OctileDistanceFP_TieBreak(Vec2i current, Vec2i target, Vec2i start) {
        int32_t dx = std::abs(current.x - target.x), dy = std::abs(current.y - target.y);
        int32_t mn = dx < dy ? dx : dy, mx = dx > dy ? dx : dy;
        int32_t h = (mx << 10) + Config::SQRT2_MINUS_1_FP * mn;

        // Tính toán độ lệch khỏi đường thẳng bằng Cross-Product
        int32_t dx1 = current.x - target.x;
        int32_t dy1 = current.y - target.y;
        int32_t dx2 = start.x - target.x;
        int32_t dy2 = start.y - target.y;
        int32_t cross = std::abs(dx1 * dy2 - dx2 * dy1);

        // Cộng 1 lượng rác cực nhỏ (cross >> 6) để bắt AI đi thẳng tắp!
        return h + (cross >> 6); 
    }

    inline int32_t GetDynamicWeightFP(int32_t startDist, int32_t h) {
        if (startDist == 0) return 1024;
        return 1024 + (Config::WEIGHT_MUL * 1024 * h / startDist);
    }
    
    inline bool HasLineOfSight(const Grid& g, Vec2i p0, Vec2i p1) {
        int32_t dx = std::abs(p1.x - p0.x), dy = -std::abs(p1.y - p0.y);
        int32_t sx = p0.x < p1.x ? 1 : -1, sy = p0.y < p1.y ? 1 : -1;
        int32_t err = dx + dy, e2;
        while (true) {
            if (!g.IsWalkable(g.ToIndex(p0))) return false;
            if (p0 == p1) break;
            e2 = 2 * err;
            if (e2 >= dy) { err += dy; p0.x += sx; }
            if (e2 <= dx) { err += dx; p0.y += sy; }
        }
        return true;
    }
}