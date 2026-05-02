#pragma once
#include <algorithm>
#include <cstdint>
#include <cstring>

#ifdef _MSC_VER
#include <intrin.h>
#define HALO_PREFETCH(ptr)                                                     \
  _mm_prefetch(reinterpret_cast<const char *>(ptr), _MM_HINT_T0)
#define HALO_ALIGN(x) __declspec(align(x))
#define LIKELY(x) (x)
#define UNLIKELY(x) (x)
#else
#define HALO_PREFETCH(ptr) __builtin_prefetch((ptr), 0, 3)
#define HALO_ALIGN(x) __attribute__((aligned(x)))

#define LIKELY(x) __builtin_expect(!!(x), 1)
#define UNLIKELY(x) __builtin_expect(!!(x), 0)
#endif

namespace halo {
struct Config {
  static constexpr int32_t FP_MULT = 1024;
  static constexpr int32_t SQRT2_FP = 1448;
  static constexpr int32_t SQRT2_MINUS_1_FP = 424;
  static constexpr int32_t WEIGHT_MUL = 3;
  static constexpr int32_t DEAD_END_PENALTY = 51200;
  static constexpr int32_t MAX_PENALTY = 512000;
  static constexpr int32_t MAX_PATH_LEN = 1024;
};

struct Vec2i {
  int32_t x, y;
  constexpr Vec2i() : x(0), y(0) {}
  constexpr Vec2i(int32_t x_, int32_t y_) : x(x_), y(y_) {}
  constexpr bool operator==(const Vec2i &o) const {
    return x == o.x && y == o.y;
  }
  constexpr bool operator!=(const Vec2i &o) const { return !(*this == o); }
  constexpr Vec2i operator+(const Vec2i &o) const { return {x + o.x, y + o.y}; }
};

struct HALO_ALIGN(32) PathNode {
  int32_t g, h, f, penalty, parent, index;
  uint32_t searchEpoch;
  uint8_t state, _pad[3];
};

class Grid {
  int32_t m_w = 0, m_h = 0;
  uint8_t *m_walk;
  int32_t *m_pen;

public:
  void Init(int32_t w, int32_t h, uint8_t *walk, int32_t *pen) {
    m_w = w;
    m_h = h;
    m_walk = walk;
    m_pen = pen;
    std::memset(m_walk, 1, w * h);
    std::memset(m_pen, 0, w * h * sizeof(int32_t));
  }
  inline int32_t ToIndex(int32_t x, int32_t y) const { return y * m_w + x; }
  inline int32_t ToIndex(Vec2i v) const { return v.y * m_w + v.x; }
  inline Vec2i ToVec(int32_t i) const { return {i % m_w, i / m_w}; }
  inline bool InBounds(Vec2i v) const {
    return LIKELY(v.x >= 0 && v.x < m_w && v.y >= 0 && v.y < m_h);
  }
  inline bool IsWalkable(int32_t i) const { return LIKELY(m_walk[i] != 0); }
  inline void SetObstacle(int32_t x, int32_t y) { m_walk[ToIndex(x, y)] = 0; }
  inline void AddPenalty(int32_t i, int32_t p) {
    m_pen[i] = std::min(m_pen[i] + p, Config::MAX_PENALTY);
  }
  inline int32_t GetPenalty(int32_t i) const { return m_pen[i]; }
  inline int32_t Size() const { return m_w * m_h; }

  inline int32_t Width() const { return m_w; }
  inline int32_t Height() const { return m_h; }
};

struct PathResult {
  bool found = false;
  int32_t cost = 0, expanded = 0, len = 0;
  Vec2i route[Config::MAX_PATH_LEN];
};

namespace Direction {
inline constexpr Vec2i Offsets[8] = {{1, 0},  {1, -1}, {0, -1}, {-1, -1},
                                     {-1, 0}, {-1, 1}, {0, 1},  {1, 1}};
inline constexpr int32_t CostFP[8] = {
    Config::FP_MULT, Config::SQRT2_FP, Config::FP_MULT, Config::SQRT2_FP,
    Config::FP_MULT, Config::SQRT2_FP, Config::FP_MULT, Config::SQRT2_FP};
inline constexpr bool IsDiag[8] = {false, true, false, true,
                                   false, true, false, true};
} // namespace Direction
} // namespace halo