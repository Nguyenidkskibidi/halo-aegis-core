#pragma once

#include <algorithm>
#include <bit>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>

#if defined(_MSC_VER)
#include <intrin.h>
#define HALO_PREFETCH(ptr) _mm_prefetch(reinterpret_cast<const char *>(ptr), _MM_HINT_T0)
#define HALO_ALIGN(x) __declspec(align(x))
#define HALO_LIKELY(x) (x)
#define HALO_UNLIKELY(x) (x)
#define HALO_INLINE __forceinline
#define HALO_RESTRICT __restrict
#else
#define HALO_PREFETCH(ptr) __builtin_prefetch((ptr), 0, 3)
#define HALO_ALIGN(x) __attribute__((aligned(x)))
#define HALO_LIKELY(x) __builtin_expect(!!(x), 1)
#define HALO_UNLIKELY(x) __builtin_expect(!!(x), 0)
#define HALO_INLINE __attribute__((always_inline)) inline
#define HALO_RESTRICT __restrict__
#endif

// Backward compatibility macros
#ifndef LIKELY
#define LIKELY(x) HALO_LIKELY(x)
#endif
#ifndef UNLIKELY
#define UNLIKELY(x) HALO_UNLIKELY(x)
#endif

// Zero-overhead C-style logging macro (disabled by default in Release/NDEBUG builds)
#if defined(HALO_ENABLE_LOGGING) && !defined(NDEBUG)
#include <cstdio>
#define HALO_LOG(...) std::printf(__VA_ARGS__)
#else
#define HALO_LOG(...) ((void)0)
#endif

namespace halo {

// Compile-Time Microarchitectural Geometry Helpers
constexpr bool IsPowerOfTwo(uint32_t n) noexcept {
  return n > 0 && (n & (n - 1)) == 0;
}

constexpr int32_t Log2Constexpr(uint32_t n) noexcept {
  return std::countr_zero(n);
}

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
  int32_t x = 0;
  int32_t y = 0;

  constexpr Vec2i() noexcept : x(0), y(0) {}
  constexpr Vec2i(int32_t x_, int32_t y_) noexcept : x(x_), y(y_) {}

  [[nodiscard]] constexpr bool operator==(const Vec2i &o) const noexcept {
    return x == o.x && y == o.y;
  }
  [[nodiscard]] constexpr bool operator!=(const Vec2i &o) const noexcept {
    return !(*this == o);
  }
  [[nodiscard]] constexpr Vec2i operator+(const Vec2i &o) const noexcept {
    return {x + o.x, y + o.y};
  }
  [[nodiscard]] constexpr Vec2i operator-(const Vec2i &o) const noexcept {
    return {x - o.x, y - o.y};
  }
  [[nodiscard]] constexpr Vec2i operator*(int32_t s) const noexcept {
    return {x * s, y * s};
  }
  constexpr Vec2i &operator+=(const Vec2i &o) noexcept {
    x += o.x;
    y += o.y;
    return *this;
  }
  constexpr Vec2i &operator-=(const Vec2i &o) noexcept {
    x -= o.x;
    y -= o.y;
    return *this;
  }
};

struct Vec2f {
  float x = 0.0f;
  float y = 0.0f;

  constexpr Vec2f() noexcept = default;
  constexpr Vec2f(float x_, float y_) noexcept : x(x_), y(y_) {}
  constexpr explicit Vec2f(const Vec2i &v) noexcept
      : x(static_cast<float>(v.x)), y(static_cast<float>(v.y)) {}

  [[nodiscard]] constexpr bool operator==(const Vec2f &o) const noexcept {
    return x == o.x && y == o.y;
  }
  [[nodiscard]] constexpr bool operator!=(const Vec2f &o) const noexcept {
    return !(*this == o);
  }
  [[nodiscard]] constexpr Vec2f operator+(const Vec2f &o) const noexcept {
    return {x + o.x, y + o.y};
  }
  [[nodiscard]] constexpr Vec2f operator-(const Vec2f &o) const noexcept {
    return {x - o.x, y - o.y};
  }
  [[nodiscard]] constexpr Vec2f operator-() const noexcept {
    return {-x, -y};
  }
  [[nodiscard]] constexpr Vec2f operator*(float s) const noexcept {
    return {x * s, y * s};
  }
  [[nodiscard]] constexpr Vec2f operator/(float s) const noexcept {
    return {x / s, y / s};
  }

  constexpr Vec2f &operator+=(const Vec2f &o) noexcept {
    x += o.x;
    y += o.y;
    return *this;
  }
  constexpr Vec2f &operator-=(const Vec2f &o) noexcept {
    x -= o.x;
    y -= o.y;
    return *this;
  }
  constexpr Vec2f &operator*=(float s) noexcept {
    x *= s;
    y *= s;
    return *this;
  }

  [[nodiscard]] float LengthSq() const noexcept {
    return x * x + y * y;
  }
  [[nodiscard]] float Length() const noexcept {
    return std::sqrt(LengthSq());
  }

  [[nodiscard]] Vec2f Normalized() const noexcept {
    float len = Length();
    return len > 1e-6f ? (*this / len) : Vec2f{0.0f, 0.0f};
  }

  [[nodiscard]] constexpr float Dot(const Vec2f &o) const noexcept {
    return x * o.x + y * o.y;
  }
  [[nodiscard]] constexpr float Cross(const Vec2f &o) const noexcept {
    return x * o.y - y * o.x;
  }

  [[nodiscard]] Vec2f Rotated(float radians) const noexcept {
    float c = std::cos(radians);
    float s = std::sin(radians);
    return {x * c - y * s, x * s + y * c};
  }

  [[nodiscard]] constexpr Vec2i ToVec2i() const noexcept {
    return {static_cast<int32_t>(x >= 0.0f ? x + 0.5f : x - 0.5f),
            static_cast<int32_t>(y >= 0.0f ? y + 0.5f : y - 0.5f)};
  }
};

[[nodiscard]] inline float AngleWrap(float rad) noexcept {
  constexpr float TWO_PI = 6.28318530717958647692f;
  while (rad > 3.14159265358979323846f) rad -= TWO_PI;
  while (rad < -3.14159265358979323846f) rad += TWO_PI;
  return rad;
}

// ============================================================================
// 3D VECTOR & MULTI-TOPOLOGY COORDINATE SYSTEMS
// ============================================================================

struct Vec3f {
  float x = 0.0f;
  float y = 0.0f;
  float z = 0.0f;

  constexpr Vec3f() noexcept = default;
  constexpr Vec3f(float x_, float y_, float z_) noexcept : x(x_), y(y_), z(z_) {}

  [[nodiscard]] constexpr bool operator==(const Vec3f &o) const noexcept {
    return x == o.x && y == o.y && z == o.z;
  }
  [[nodiscard]] constexpr bool operator!=(const Vec3f &o) const noexcept {
    return !(*this == o);
  }
  [[nodiscard]] constexpr Vec3f operator+(const Vec3f &o) const noexcept {
    return {x + o.x, y + o.y, z + o.z};
  }
  [[nodiscard]] constexpr Vec3f operator-(const Vec3f &o) const noexcept {
    return {x - o.x, y - o.y, z - o.z};
  }
  [[nodiscard]] constexpr Vec3f operator-() const noexcept {
    return {-x, -y, -z};
  }
  [[nodiscard]] constexpr Vec3f operator*(float s) const noexcept {
    return {x * s, y * s, z * s};
  }
  [[nodiscard]] constexpr Vec3f operator/(float s) const noexcept {
    return {x / s, y / s, z / s};
  }
  constexpr Vec3f &operator+=(const Vec3f &o) noexcept {
    x += o.x; y += o.y; z += o.z; return *this;
  }
  constexpr Vec3f &operator-=(const Vec3f &o) noexcept {
    x -= o.x; y -= o.y; z -= o.z; return *this;
  }
  constexpr Vec3f &operator*=(float s) noexcept {
    x *= s; y *= s; z *= s; return *this;
  }

  [[nodiscard]] float LengthSq() const noexcept {
    return x * x + y * y + z * z;
  }
  [[nodiscard]] float Length() const noexcept {
    return std::sqrt(LengthSq());
  }
  [[nodiscard]] Vec3f Normalized() const noexcept {
    float len = Length();
    return len > 1e-6f ? (*this / len) : Vec3f{0.0f, 0.0f, 0.0f};
  }
  [[nodiscard]] constexpr float Dot(const Vec3f &o) const noexcept {
    return x * o.x + y * o.y + z * o.z;
  }
  [[nodiscard]] constexpr Vec3f Cross(const Vec3f &o) const noexcept {
    return {y * o.z - z * o.y, z * o.x - x * o.z, x * o.y - y * o.x};
  }
};

// Hexagonal Axial (q, r) and Cube (q, r, s) Coordinates
// Invariant: q + r + s == 0
struct HexCoord {
  int32_t q = 0;
  int32_t r = 0;

  constexpr HexCoord() noexcept = default;
  constexpr HexCoord(int32_t q_, int32_t r_) noexcept : q(q_), r(r_) {}

  [[nodiscard]] constexpr int32_t S() const noexcept { return -q - r; }

  [[nodiscard]] constexpr bool operator==(const HexCoord &o) const noexcept {
    return q == o.q && r == o.r;
  }
  [[nodiscard]] constexpr bool operator!=(const HexCoord &o) const noexcept {
    return !(*this == o);
  }
  [[nodiscard]] constexpr HexCoord operator+(const HexCoord &o) const noexcept {
    return {q + o.q, r + o.r};
  }
  [[nodiscard]] constexpr HexCoord operator-(const HexCoord &o) const noexcept {
    return {q - o.q, r - o.r};
  }

  // Hex Manhattan distance: (|q| + |r| + |s|) / 2
  [[nodiscard]] constexpr int32_t DistanceTo(const HexCoord &o) const noexcept {
    int32_t dq = std::abs(q - o.q);
    int32_t dr = std::abs(r - o.r);
    int32_t ds = std::abs(S() - o.S());
    return (dq + dr + ds) / 2;
  }

  [[nodiscard]] constexpr HexCoord Neighbor(int32_t dir) const noexcept;

  // Convert to 2D Cartesian world coordinates (flat-top hex layout)
  [[nodiscard]] Vec2f ToWorld(float radius = 1.0f) const noexcept {
    float x = radius * (1.5f * static_cast<float>(q));
    float y = radius * (1.7320508075688772f * (static_cast<float>(r) + 0.5f * static_cast<float>(q)));
    return {x, y};
  }
};

inline constexpr HexCoord HEX_NEIGHBOR_OFFSETS[6] = {
    {1, 0}, {1, -1}, {0, -1}, {-1, 0}, {-1, 1}, {0, 1}
};

inline constexpr HexCoord HexCoord::Neighbor(int32_t dir) const noexcept {
  return *this + HEX_NEIGHBOR_OFFSETS[dir % 6];
}

// 2.5D Multi-Floor Coordinate for multi-story buildings, terrain bridges, ramps
struct FloorCoord {
  int32_t x = 0;
  int32_t y = 0;
  int32_t floor = 0;

  constexpr FloorCoord() noexcept = default;
  constexpr FloorCoord(int32_t x_, int32_t y_, int32_t f_ = 0) noexcept : x(x_), y(y_), floor(f_) {}

  [[nodiscard]] constexpr bool operator==(const FloorCoord &o) const noexcept {
    return x == o.x && y == o.y && floor == o.floor;
  }
  [[nodiscard]] constexpr bool operator!=(const FloorCoord &o) const noexcept {
    return !(*this == o);
  }
  [[nodiscard]] constexpr Vec2i ToVec2i() const noexcept { return {x, y}; }
};

// 64-bit Bit-Packed Voxel Coordinates: 21-bit X, 21-bit Y, 22-bit Z
struct VoxelCoord {
  int32_t x = 0;
  int32_t y = 0;
  int32_t z = 0;

  constexpr VoxelCoord() noexcept = default;
  constexpr VoxelCoord(int32_t x_, int32_t y_, int32_t z_) noexcept : x(x_), y(y_), z(z_) {}

  [[nodiscard]] constexpr bool operator==(const VoxelCoord &o) const noexcept {
    return x == o.x && y == o.y && z == o.z;
  }
  [[nodiscard]] constexpr bool operator!=(const VoxelCoord &o) const noexcept {
    return !(*this == o);
  }
  [[nodiscard]] constexpr VoxelCoord operator+(const VoxelCoord &o) const noexcept {
    return {x + o.x, y + o.y, z + o.z};
  }
  [[nodiscard]] constexpr VoxelCoord operator-(const VoxelCoord &o) const noexcept {
    return {x - o.x, y - o.y, z - o.z};
  }

  [[nodiscard]] uint64_t Pack() const noexcept {
    uint64_t ux = static_cast<uint64_t>(x) & 0x1FFFFFULL;
    uint64_t uy = static_cast<uint64_t>(y) & 0x1FFFFFULL;
    uint64_t uz = static_cast<uint64_t>(z) & 0x3FFFFFULL;
    return ux | (uy << 21) | (uz << 42);
  }

  static constexpr VoxelCoord Unpack(uint64_t key) noexcept {
    int32_t rawX = static_cast<int32_t>(key & 0x1FFFFFULL);
    int32_t rawY = static_cast<int32_t>((key >> 21) & 0x1FFFFFULL);
    int32_t rawZ = static_cast<int32_t>((key >> 42) & 0x3FFFFFULL);
    // Sign-extension for 21-bit X, 21-bit Y, 22-bit Z
    if (rawX & (1 << 20)) rawX |= ~0x1FFFFF;
    if (rawY & (1 << 20)) rawY |= ~0x1FFFFF;
    if (rawZ & (1 << 21)) rawZ |= ~0x3FFFFF;
    return {rawX, rawY, rawZ};
  }

  [[nodiscard]] int32_t ManhattanDistance(const VoxelCoord &o) const noexcept {
    return std::abs(x - o.x) + std::abs(y - o.y) + std::abs(z - o.z);
  }
  [[nodiscard]] int32_t ChebyshevDistance(const VoxelCoord &o) const noexcept {
    return std::max({std::abs(x - o.x), std::abs(y - o.y), std::abs(z - o.z)});
  }
  [[nodiscard]] float EuclideanDistance(const VoxelCoord &o) const noexcept {
    float dx = static_cast<float>(x - o.x);
    float dy = static_cast<float>(y - o.y);
    float dz = static_cast<float>(z - o.z);
    return std::sqrt(dx * dx + dy * dy + dz * dz);
  }

  [[nodiscard]] constexpr VoxelCoord Neighbor6(int32_t dir) const noexcept;
};

inline constexpr VoxelCoord VOXEL_NEIGHBORS_6[6] = {
    {1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}
};

inline constexpr VoxelCoord VoxelCoord::Neighbor6(int32_t dir) const noexcept {
  return *this + VOXEL_NEIGHBORS_6[dir % 6];
}

struct alignas(32) PathNode {
  int32_t g = 0;
  int32_t h = 0;
  int32_t f = 0;
  int32_t penalty = 0;
  int32_t parent = -1;
  int32_t index = -1;
  uint32_t searchEpoch = 0;
  uint8_t state = 0; // 0 = unvisited, 1 = open, 2 = closed
  uint8_t _pad[3] = {0, 0, 0};
};
static_assert(sizeof(PathNode) == 32, "PathNode must be exactly 32 bytes for cache-line alignment");

// NTTP Compile-Time Geometry Baked-in Grid
// Annihilates runtime integer multiplications/divisions for power-of-two geometries
template <int32_t W = 0, int32_t H = 0>
class alignas(64) GridT {
private:
  int32_t m_w = W;
  int32_t m_h = H;
  uint8_t *m_walk = nullptr;
  int32_t *m_pen = nullptr;

  static constexpr bool HAS_STATIC_DIM = (W > 0 && H > 0);
  static constexpr bool IS_POW2 = HAS_STATIC_DIM && IsPowerOfTwo(W);
  static constexpr int32_t SHIFT_W = IS_POW2 ? Log2Constexpr(W) : 0;
  static constexpr int32_t MASK_W = IS_POW2 ? (W - 1) : 0;

public:
  GridT() noexcept = default;

  void Init(int32_t w, int32_t h, uint8_t *walk, int32_t *pen) noexcept {
    m_w = w;
    m_h = h;
    m_walk = walk;
    m_pen = pen;
    if (m_walk) {
      std::memset(m_walk, 1, static_cast<size_t>(w) * h);
    }
    if (m_pen) {
      std::memset(m_pen, 0, static_cast<size_t>(w) * h * sizeof(int32_t));
    }
  }

  [[nodiscard]] HALO_INLINE int32_t ToIndex(int32_t x, int32_t y) const noexcept {
    if constexpr (IS_POW2) {
      return (y << SHIFT_W) | x;
    } else {
      return y * m_w + x;
    }
  }

  [[nodiscard]] HALO_INLINE int32_t ToIndex(Vec2i v) const noexcept {
    return ToIndex(v.x, v.y);
  }

  [[nodiscard]] HALO_INLINE Vec2i ToVec(int32_t i) const noexcept {
    if constexpr (IS_POW2) {
      return {i & MASK_W, i >> SHIFT_W};
    } else {
      return {i % m_w, i / m_w};
    }
  }

  [[nodiscard]] HALO_INLINE bool InBounds(int32_t x, int32_t y) const noexcept {
    if constexpr (HAS_STATIC_DIM) {
      return static_cast<uint32_t>(x) < static_cast<uint32_t>(W) &&
             static_cast<uint32_t>(y) < static_cast<uint32_t>(H);
    } else {
      return static_cast<uint32_t>(x) < static_cast<uint32_t>(m_w) &&
             static_cast<uint32_t>(y) < static_cast<uint32_t>(m_h);
    }
  }

  [[nodiscard]] HALO_INLINE bool InBounds(Vec2i v) const noexcept {
    return InBounds(v.x, v.y);
  }

  [[nodiscard]] HALO_INLINE bool IsWalkable(int32_t i) const noexcept {
    return HALO_LIKELY(m_walk != nullptr && m_walk[i] != 0);
  }

  [[nodiscard]] HALO_INLINE bool IsWalkable(int32_t x, int32_t y) const noexcept {
    if (HALO_UNLIKELY(!InBounds(x, y))) return false;
    return IsWalkable(ToIndex(x, y));
  }

  [[nodiscard]] HALO_INLINE bool IsWalkable(Vec2i v) const noexcept {
    return IsWalkable(v.x, v.y);
  }

  HALO_INLINE void SetObstacle(int32_t x, int32_t y) noexcept {
    if (HALO_LIKELY(InBounds(x, y))) {
      m_walk[ToIndex(x, y)] = 0;
    }
  }

  HALO_INLINE void SetWalkable(int32_t x, int32_t y, bool walkable) noexcept {
    if (HALO_LIKELY(InBounds(x, y))) {
      m_walk[ToIndex(x, y)] = walkable ? 1 : 0;
    }
  }

  HALO_INLINE void AddPenalty(int32_t i, int32_t p) noexcept {
    if (m_pen) {
      m_pen[i] = std::min(m_pen[i] + p, Config::MAX_PENALTY);
    }
  }

  [[nodiscard]] HALO_INLINE int32_t GetPenalty(int32_t i) const noexcept {
    return m_pen ? m_pen[i] : 0;
  }

  [[nodiscard]] HALO_INLINE int32_t Size() const noexcept {
    if constexpr (HAS_STATIC_DIM) return W * H;
    else return m_w * m_h;
  }

  [[nodiscard]] HALO_INLINE int32_t Width() const noexcept {
    if constexpr (HAS_STATIC_DIM) return W;
    else return m_w;
  }

  [[nodiscard]] HALO_INLINE int32_t Height() const noexcept {
    if constexpr (HAS_STATIC_DIM) return H;
    else return m_h;
  }

  [[nodiscard]] inline const uint8_t *GetWalkBuffer() const noexcept { return m_walk; }
  [[nodiscard]] inline uint8_t *GetWalkBuffer() noexcept { return m_walk; }
};

// Dynamic Grid alias for backward-compatible dynamic-size usage
using Grid = GridT<0, 0>;

struct PathResult {
  bool found = false;
  int32_t cost = 0;
  int32_t expanded = 0;
  int32_t len = 0;
  Vec2i route[Config::MAX_PATH_LEN];
};

namespace Direction {
inline constexpr int32_t EAST = 0;
inline constexpr int32_t NORTHEAST = 1;
inline constexpr int32_t NORTH = 2;
inline constexpr int32_t NORTHWEST = 3;
inline constexpr int32_t WEST = 4;
inline constexpr int32_t SOUTHWEST = 5;
inline constexpr int32_t SOUTH = 6;
inline constexpr int32_t SOUTHEAST = 7;

inline constexpr Vec2i Offsets[8] = {
    {1, 0},   // 0: East
    {1, -1},  // 1: NorthEast
    {0, -1},  // 2: North
    {-1, -1}, // 3: NorthWest
    {-1, 0},  // 4: West
    {-1, 1},  // 5: SouthWest
    {0, 1},   // 6: South
    {1, 1}    // 7: SouthEast
};

inline constexpr int32_t CostFP[8] = {
    Config::FP_MULT, Config::SQRT2_FP, Config::FP_MULT, Config::SQRT2_FP,
    Config::FP_MULT, Config::SQRT2_FP, Config::FP_MULT, Config::SQRT2_FP
};

inline constexpr bool IsDiag[8] = {
    false, true, false, true,
    false, true, false, true
};

inline constexpr bool IsDiagonal[8] = {
    false, true, false, true,
    false, true, false, true
};

inline constexpr int32_t Opposite[8] = {4, 5, 6, 7, 0, 1, 2, 3};
} // namespace Direction

} // namespace halo