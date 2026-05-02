#pragma once

#include "../utils/halo_types.h"
#include <cstdint>
#include <queue>
#include <vector>

namespace halo::swar {
class UltimateBitboard64;
}

namespace halo::swarm {

class FlowField {
public:
  void Generate(const swar::UltimateBitboard64 &grid, Vec2i target) {
    int32_t totalSize = 64 * 64;
    m_integrationField.assign(totalSize, 65535);
    m_vectorField.assign(totalSize, -1);

    int32_t targetIdx = target.y * 64 + target.x;
    m_integrationField[targetIdx] = 0;

    std::queue<int32_t> frontier;
    frontier.push(targetIdx);

    while (!frontier.empty()) {
      int32_t currIdx = frontier.front();
      frontier.pop();

      int32_t currX = currIdx % 64;
      int32_t currY = currIdx / 64;

      for (int i = 0; i < 8; ++i) {
        Vec2i nextPos = Vec2i(currX, currY) + Direction::Offsets[i];

        if (nextPos.x < 0 || nextPos.x >= 64 || nextPos.y < 0 ||
            nextPos.y >= 64)
          continue;

        int32_t nextIdx = nextPos.y * 64 + nextPos.x;

        uint16_t moveCost = Direction::IsDiagonal[i] ? 14 : 10;
        uint16_t newCost = m_integrationField[currIdx] + moveCost;

        if (newCost < m_integrationField[nextIdx]) {
          m_integrationField[nextIdx] = newCost;
          m_vectorField[nextIdx] = (int8_t)((i + 4) % 8);
          frontier.push(nextIdx);
        }
      }
    }
  }

  inline int8_t GetDirectionForAgent(int32_t idx) const noexcept {
    if (idx < 0 || idx >= (int32_t)m_vectorField.size())
      return -1;
    return m_vectorField[idx];
  }

private:
  std::vector<uint16_t> m_integrationField;
  std::vector<int8_t> m_vectorField;
};

} // namespace halo::swarm