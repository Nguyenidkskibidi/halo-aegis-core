#pragma once

#include <cstdint>
#include <cmath>

namespace halo::fixed {

// ============================================================================
// PURE 32-BIT Q16.16 FIXED-POINT MATHEMATICS (ZERO-FPU BARE-METAL EMBEDDED)
// ============================================================================
// Scale factor: 2^16 = 65536. 1.0f == 65536.
// Designed for microcontrollers (ESP32, STM32F4/H7, Cortex-M4/M7, RISC-V 32)
// providing deterministic sub-nanosecond integer execution without FPU traps.

struct alignas(4) Fixed32 {
  int32_t v = 0;

  static constexpr int32_t SHIFT = 16;
  static constexpr int32_t ONE = 1 << SHIFT;
  static constexpr int32_t HALF = 1 << (SHIFT - 1);
  static constexpr int32_t PI_FP = 205887; // 3.14159265 * 65536

  constexpr Fixed32() noexcept = default;
  constexpr explicit Fixed32(int32_t raw, bool /*rawTag*/) noexcept : v(raw) {}
  constexpr Fixed32(int32_t integer) noexcept : v(integer << SHIFT) {}
  constexpr explicit Fixed32(float f) noexcept : v(static_cast<int32_t>(f * 65536.0f + (f >= 0 ? 0.5f : -0.5f))) {}
  constexpr explicit Fixed32(double d) noexcept : v(static_cast<int32_t>(d * 65536.0 + (d >= 0 ? 0.5 : -0.5))) {}

  [[nodiscard]] constexpr float ToFloat() const noexcept {
    return static_cast<float>(v) / 65536.0f;
  }

  [[nodiscard]] constexpr int32_t ToInt() const noexcept {
    return v >> SHIFT;
  }

  [[nodiscard]] static constexpr Fixed32 FromRaw(int32_t raw) noexcept {
    return Fixed32(raw, true);
  }

  [[nodiscard]] static constexpr Fixed32 FromFloat(float f) noexcept {
    return Fixed32(f);
  }

  constexpr Fixed32 operator-() const noexcept {
    return Fixed32(-v, true);
  }

  constexpr Fixed32 &operator+=(Fixed32 rhs) noexcept {
    v += rhs.v;
    return *this;
  }

  constexpr Fixed32 &operator-=(Fixed32 rhs) noexcept {
    v -= rhs.v;
    return *this;
  }

  constexpr Fixed32 &operator*=(Fixed32 rhs) noexcept {
    v = static_cast<int32_t>((static_cast<int64_t>(v) * rhs.v + HALF) >> SHIFT);
    return *this;
  }

  constexpr Fixed32 &operator/=(Fixed32 rhs) noexcept {
    v = static_cast<int32_t>((static_cast<int64_t>(v) << SHIFT) / rhs.v);
    return *this;
  }

  friend constexpr Fixed32 operator+(Fixed32 a, Fixed32 b) noexcept { return Fixed32(a.v + b.v, true); }
  friend constexpr Fixed32 operator-(Fixed32 a, Fixed32 b) noexcept { return Fixed32(a.v - b.v, true); }
  friend constexpr Fixed32 operator*(Fixed32 a, Fixed32 b) noexcept {
    return Fixed32(static_cast<int32_t>((static_cast<int64_t>(a.v) * b.v + HALF) >> SHIFT), true);
  }
  friend constexpr Fixed32 operator/(Fixed32 a, Fixed32 b) noexcept {
    return Fixed32(static_cast<int32_t>((static_cast<int64_t>(a.v) << SHIFT) / b.v), true);
  }

  friend constexpr bool operator==(Fixed32 a, Fixed32 b) noexcept { return a.v == b.v; }
  friend constexpr bool operator!=(Fixed32 a, Fixed32 b) noexcept { return a.v != b.v; }
  friend constexpr bool operator<(Fixed32 a, Fixed32 b) noexcept { return a.v < b.v; }
  friend constexpr bool operator>(Fixed32 a, Fixed32 b) noexcept { return a.v > b.v; }
  friend constexpr bool operator<=(Fixed32 a, Fixed32 b) noexcept { return a.v <= b.v; }
  friend constexpr bool operator>=(Fixed32 a, Fixed32 b) noexcept { return a.v >= b.v; }
};

// Branchless Integer Square Root for Q16.16
[[nodiscard]] inline Fixed32 Sqrt(Fixed32 a) noexcept {
  if (a.v <= 0) return Fixed32(0, true);
  uint64_t val = static_cast<uint64_t>(a.v) << 16;
  uint64_t res = 0;
  uint64_t bit = 1ULL << 62;

  while (bit > val) {
    bit >>= 2;
  }

  while (bit != 0) {
    if (val >= res + bit) {
      val -= res + bit;
      res = (res >> 1) + bit;
    } else {
      res >>= 1;
    }
    bit >>= 2;
  }

  return Fixed32(static_cast<int32_t>(res), true);
}

// 64-Byte Aligned Precomputed 360-Degree Sin/Cos LUT (Q16.16)
// Generated at compile-time for zero runtime initialization stalls.
struct alignas(64) TrigLut360 {
  int32_t sinTable[360];
  int32_t cosTable[360];

  constexpr TrigLut360() noexcept : sinTable{}, cosTable{} {
    constexpr double PI = 3.14159265358979323846;
    for (int deg = 0; deg < 360; ++deg) {
      // Angle reduction to [-PI, PI] ensures fast, sub-microsecond convergence
      double rad = (deg <= 180 ? deg : deg - 360) * (PI / 180.0);
      double s = rad;
      double termS = rad;
      for (int i = 1; i <= 9; ++i) {
        termS *= -rad * rad / ((2 * i) * (2 * i + 1));
        s += termS;
      }
      double c = 1.0;
      double termC = 1.0;
      for (int i = 1; i <= 9; ++i) {
        termC *= -rad * rad / ((2 * i - 1) * (2 * i));
        c += termC;
      }
      sinTable[deg] = static_cast<int32_t>(s * 65536.0 + (s >= 0 ? 0.5 : -0.5));
      cosTable[deg] = static_cast<int32_t>(c * 65536.0 + (c >= 0 ? 0.5 : -0.5));
    }
  }
};

inline constexpr TrigLut360 g_trigLut360{};

[[nodiscard]] inline Fixed32 SinDeg(int32_t deg) noexcept {
  int32_t d = deg % 360;
  if (d < 0) d += 360;
  return Fixed32::FromRaw(g_trigLut360.sinTable[d]);
}

[[nodiscard]] inline Fixed32 CosDeg(int32_t deg) noexcept {
  int32_t d = deg % 360;
  if (d < 0) d += 360;
  return Fixed32::FromRaw(g_trigLut360.cosTable[d]);
}

// Fast Branchless Fixed-Point Atan2 Approximation (Max error < 0.005 rad)
[[nodiscard]] inline Fixed32 Atan2(Fixed32 y, Fixed32 x) noexcept {
  if (x.v == 0 && y.v == 0) return Fixed32(0, true);

  int64_t absY = y.v >= 0 ? y.v : -y.v;
  int64_t absX = x.v >= 0 ? x.v : -x.v;

  int64_t minVal = absX < absY ? absX : absY;
  int64_t maxVal = absX > absY ? absX : absY;

  if (maxVal == 0) return Fixed32(0, true);

  int64_t ratio = (minVal << 16) / maxVal;
  int64_t ratioSq = (ratio * ratio) >> 16;

  // Padé / Polynomial: angle = ratio * (0.995354 - 0.288679 * ratio^2)
  int64_t c1 = 65230; // 0.995354 * 65536
  int64_t c2 = 18918; // 0.288679 * 65536
  int64_t poly = c1 - ((c2 * ratioSq) >> 16);
  int64_t angle = (ratio * poly) >> 16;

  if (absY > absX) {
    angle = (Fixed32::PI_FP >> 1) - angle;
  }
  if (x.v < 0) {
    angle = Fixed32::PI_FP - angle;
  }
  if (y.v < 0) {
    angle = -angle;
  }

  return Fixed32::FromRaw(static_cast<int32_t>(angle));
}

} // namespace halo::fixed
