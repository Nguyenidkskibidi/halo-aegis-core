#pragma once

#include "halo_graph.h"
#include <cmath>
#include <cstdint>

namespace halo::urban {

class WormholeEngine {
public:
  static int32_t DigWormholes(CityMap &map) noexcept {
    const int32_t nodes = map.GetNodeCount();
    const CityIntersection *intersections = map.GetNodes();
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
    return wormholesCreated;
  }
};

} // namespace halo::urban