#pragma once
#include "halo_graph.h"
#include <iostream>

namespace halo::urban {

class WormholeEngine {
public:
  static void DigWormholes(CityMap &map) noexcept {
    int32_t nodes = map.GetNodeCount();
    CityIntersection *intersections = map.GetNodes();
    int32_t wormholesCreated = 0;

    for (int32_t i = 0; i < nodes; ++i) {
      for (int32_t j = i + 1; j < nodes; ++j) {
        int32_t dx = std::abs(intersections[i].x_fp - intersections[j].x_fp);
        int32_t dy = std::abs(intersections[i].y_fp - intersections[j].y_fp);

        if (dx + dy > 15360) {
          map.AddTwoWay(i, j, 0.5f);
          wormholesCreated++;
        }
      }
    }
    std::cout << "🌌 [WORMHOLE ENGINE]: Đã đào thành công " << wormholesCreated
              << " Lỗ Sâu Không Gian!\n";
  }
};

} // namespace halo::urban