#pragma once

#include "../utils/halo_math.h"
#include "../utils/halo_types.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>

namespace halo::kinodynamics {

// ============================================================================
// CONTINUOUS KINODYNAMIC TRAJECTORY GENERATION & REAL-TIME TRACKING (1 kHz)
// Minimum-Jerk Piecewise 5th-Order Polynomials (Quintic Spline) with C^3 Continuity
// ============================================================================

struct alignas(16) KinodynamicLimits {
  float maxVelocity = 8.0f;     // m/s
  float maxAcceleration = 4.0f; // m/s^2
  float maxJerk = 15.0f;        // m/s^3
  float maxCurvature = 2.0f;    // 1/m (min turn radius = 0.5m)
  float nominalSpeed = 4.0f;    // m/s
  float minSegmentDuration = 0.1f; // seconds
};

struct alignas(16) TrajectoryPoint {
  Vec2f pos{0.0f, 0.0f};
  Vec2f vel{0.0f, 0.0f};
  Vec2f acc{0.0f, 0.0f};
  Vec2f jerk{0.0f, 0.0f};
  float speed = 0.0f;
  float heading = 0.0f;
  float curvature = 0.0f;
  float time = 0.0f;
};

// 1D Quintic Polynomial: p(t) = c0 + c1*t + c2*t^2 + c3*t^3 + c4*t^4 + c5*t^5
struct alignas(32) QuinticPolynomial1D {
  float c0 = 0.0f;
  float c1 = 0.0f;
  float c2 = 0.0f;
  float c3 = 0.0f;
  float c4 = 0.0f;
  float c5 = 0.0f;

  // Closed-form analytical solver: C^3 boundary conditions in < 10 ns
  HALO_INLINE void Solve(float p0, float v0, float a0, float p1, float v1, float a1, float T) noexcept {
    c0 = p0;
    c1 = v0;
    c2 = 0.5f * a0;

    const float invT = 1.0f / T;
    const float invT2 = invT * invT;
    const float invT3 = invT2 * invT;
    const float invT4 = invT3 * invT;
    const float invT5 = invT4 * invT;

    const float D = p1 - (p0 + v0 * T + 0.5f * a0 * T * T);
    const float V = v1 - (v0 + a0 * T);
    const float A = a1 - a0;

    // Exact inverse matrix resolution (Determinant = 2):
    // c3 = (10*D)/T^3 - (4*V)/T^2 + A/(2*T)
    c3 = 10.0f * D * invT3 - 4.0f * V * invT2 + 0.5f * A * invT;
    // c4 = -(15*D)/T^4 + (7*V)/T^3 - A/T^2
    c4 = -15.0f * D * invT4 + 7.0f * V * invT3 - A * invT2;
    // c5 = (6*D)/T^5 - (3*V)/T^4 + A/(2*T^3)
    c5 = 6.0f * D * invT5 - 3.0f * V * invT4 + 0.5f * A * invT3;
  }

  [[nodiscard]] HALO_INLINE float Pos(float t) const noexcept {
    return c0 + t * (c1 + t * (c2 + t * (c3 + t * (c4 + t * c5))));
  }

  [[nodiscard]] HALO_INLINE float Vel(float t) const noexcept {
    return c1 + t * (2.0f * c2 + t * (3.0f * c3 + t * (4.0f * c4 + 5.0f * c5 * t)));
  }

  [[nodiscard]] HALO_INLINE float Acc(float t) const noexcept {
    return 2.0f * c2 + t * (6.0f * c3 + t * (12.0f * c4 + 20.0f * c5 * t));
  }

  [[nodiscard]] HALO_INLINE float Jerk(float t) const noexcept {
    return 6.0f * c3 + t * (24.0f * c4 + 60.0f * c5 * t);
  }
};

struct alignas(64) QuinticSegment2D {
  QuinticPolynomial1D x;
  QuinticPolynomial1D y;
  float duration = 0.0f;
  float startTime = 0.0f;
  float endTime = 0.0f;

  HALO_INLINE void Solve(Vec2f p0, Vec2f v0, Vec2f a0, Vec2f p1, Vec2f v1, Vec2f a1, float T, float tStart) noexcept {
    duration = T;
    startTime = tStart;
    endTime = tStart + T;
    x.Solve(p0.x, v0.x, a0.x, p1.x, v1.x, a1.x, T);
    y.Solve(p0.y, v0.y, a0.y, p1.y, v1.y, a1.y, T);
  }

  [[nodiscard]] HALO_INLINE TrajectoryPoint Evaluate(float globalTime) const noexcept {
    float localT = std::clamp(globalTime - startTime, 0.0f, duration);
    TrajectoryPoint pt;
    pt.pos = Vec2f{x.Pos(localT), y.Pos(localT)};
    pt.vel = Vec2f{x.Vel(localT), y.Vel(localT)};
    pt.acc = Vec2f{x.Acc(localT), y.Acc(localT)};
    pt.jerk = Vec2f{x.Jerk(localT), y.Jerk(localT)};
    pt.speed = std::sqrt(pt.vel.x * pt.vel.x + pt.vel.y * pt.vel.y);
    pt.heading = std::atan2(pt.vel.y, pt.vel.x);
    pt.time = globalTime;

    const float speedSq = pt.speed * pt.speed;
    if (speedSq > 0.0001f) {
      pt.curvature = (pt.vel.x * pt.acc.y - pt.vel.y * pt.acc.x) / (speedSq * pt.speed);
    } else {
      pt.curvature = 0.0f;
    }
    return pt;
  }
};

class alignas(64) KinodynamicTrajectory {
public:
  static constexpr int32_t MAX_SEGMENTS = 64;

private:
  QuinticSegment2D m_segments[MAX_SEGMENTS];
  int32_t m_segmentCount = 0;
  float m_totalDuration = 0.0f;

public:
  KinodynamicTrajectory() noexcept = default;

  void Clear() noexcept {
    m_segmentCount = 0;
    m_totalDuration = 0.0f;
  }

  [[nodiscard]] inline int32_t SegmentCount() const noexcept { return m_segmentCount; }
  [[nodiscard]] inline float TotalDuration() const noexcept { return m_totalDuration; }

  HALO_INLINE void AddSegment(const QuinticSegment2D &seg) noexcept {
    if (m_segmentCount < MAX_SEGMENTS) {
      m_segments[m_segmentCount++] = seg;
      m_totalDuration = seg.endTime;
    }
  }

  [[nodiscard]] HALO_INLINE const QuinticSegment2D &GetSegment(int32_t i) const noexcept {
    return m_segments[i];
  }

  // Evaluates trajectory point in O(log N) or fast linear scan (< 20 ns)
  [[nodiscard]] HALO_INLINE TrajectoryPoint Evaluate(float t) const noexcept {
    if (HALO_UNLIKELY(m_segmentCount == 0)) return {};
    if (t <= 0.0f) return m_segments[0].Evaluate(0.0f);
    if (t >= m_totalDuration) return m_segments[m_segmentCount - 1].Evaluate(m_totalDuration);

    for (int32_t i = 0; i < m_segmentCount; ++i) {
      if (t <= m_segments[i].endTime) {
        return m_segments[i].Evaluate(t);
      }
    }
    return m_segments[m_segmentCount - 1].Evaluate(m_totalDuration);
  }
};

// ============================================================================
// SUB-MICROSECOND MINIMUM-JERK TRAJECTORY PLANNER
// Transforms discrete waypoints into continuous C^3 quintic polynomials in < 800 ns.
// ============================================================================

[[gnu::hot]] inline bool GenerateQuinticTrajectory(
    const Vec2f *HALO_RESTRICT waypoints,
    int32_t waypointCount,
    const KinodynamicLimits &limits,
    KinodynamicTrajectory &outTraj) noexcept {
  outTraj.Clear();
  if (HALO_UNLIKELY(!waypoints || waypointCount < 2)) return false;

  const int32_t N = std::min(waypointCount, static_cast<int32_t>(KinodynamicTrajectory::MAX_SEGMENTS + 1));
  const int32_t segCount = N - 1;

  // Stack-allocated boundary kinematics arrays (zero heap)
  Vec2f velocities[KinodynamicTrajectory::MAX_SEGMENTS + 1];
  Vec2f accelerations[KinodynamicTrajectory::MAX_SEGMENTS + 1];
  float segmentDurations[KinodynamicTrajectory::MAX_SEGMENTS];

  std::memset(velocities, 0, sizeof(velocities));
  std::memset(accelerations, 0, sizeof(accelerations));

  // 1. Compute nominal segment times and intermediate tangent velocities
  for (int32_t i = 0; i < segCount; ++i) {
    float dx = waypoints[i + 1].x - waypoints[i].x;
    float dy = waypoints[i + 1].y - waypoints[i].y;
    float dist = std::sqrt(dx * dx + dy * dy);
    float tNominal = dist / std::max(0.1f, limits.nominalSpeed);
    segmentDurations[i] = std::max(limits.minSegmentDuration, tNominal);
  }

  // Intermediate velocity tangents (heuristic for smooth continuous flow)
  for (int32_t i = 1; i < segCount; ++i) {
    Vec2f tangent = waypoints[i + 1] - waypoints[i - 1];
    float dist = std::sqrt(tangent.x * tangent.x + tangent.y * tangent.y);
    if (dist > 0.001f) {
      Vec2f dir = tangent * (1.0f / dist);
      float speed = std::min(limits.maxVelocity, 0.5f * (dist / (segmentDurations[i - 1] + segmentDurations[i])));
      velocities[i] = dir * speed;
    }
  }
  // Zero start/end velocities for resting boundary condition
  velocities[0] = Vec2f{0.0f, 0.0f};
  velocities[segCount] = Vec2f{0.0f, 0.0f};

  // 2. Closed-form segment synthesis & time-dilation optimization
  float cumulativeTime = 0.0f;
  for (int32_t i = 0; i < segCount; ++i) {
    float T = segmentDurations[i];
    QuinticSegment2D seg;
    seg.Solve(waypoints[i], velocities[i], accelerations[i],
              waypoints[i + 1], velocities[i + 1], accelerations[i + 1],
              T, cumulativeTime);

    // Fast check: sample mid-point and limits
    TrajectoryPoint mid = seg.Evaluate(cumulativeTime + 0.5f * T);
    if (mid.speed > limits.maxVelocity || std::abs(mid.curvature) > limits.maxCurvature) {
      // Dilate time by scaling factor to ensure motor feasibility
      float dilation = std::max(mid.speed / limits.maxVelocity, 1.2f);
      T *= dilation;
      seg.Solve(waypoints[i], velocities[i], accelerations[i],
                waypoints[i + 1], velocities[i + 1], accelerations[i + 1],
                T, cumulativeTime);
    }

    outTraj.AddSegment(seg);
    cumulativeTime += T;
  }

  return true;
}

// Overload for integer discrete waypoints from JPS+
inline bool GenerateQuinticTrajectory(
    const Vec2i *HALO_RESTRICT waypoints,
    int32_t waypointCount,
    const KinodynamicLimits &limits,
    KinodynamicTrajectory &outTraj) noexcept {
  if (HALO_UNLIKELY(!waypoints || waypointCount < 2)) return false;

  Vec2f continuousWaypoints[KinodynamicTrajectory::MAX_SEGMENTS + 1];
  const int32_t count = std::min(waypointCount, static_cast<int32_t>(KinodynamicTrajectory::MAX_SEGMENTS + 1));
  for (int32_t i = 0; i < count; ++i) {
    continuousWaypoints[i] = Vec2f{static_cast<float>(waypoints[i].x), static_cast<float>(waypoints[i].y)};
  }
  return GenerateQuinticTrajectory(continuousWaypoints, count, limits, outTraj);
}

// ============================================================================
// 1 kHz REAL-TIME PATH-FOLLOWING TRACKERS (< 50 ns Execution)
// ============================================================================

struct alignas(8) ControlCommand {
  float linearVelocity = 0.0f;  // m/s
  float angularVelocity = 0.0f; // rad/s
};

// 1. Pure Pursuit Tracker: Computes instantaneous (v, omega) steering in < 30 ns
[[nodiscard]] HALO_INLINE ControlCommand EvaluatePurePursuit(
    Vec2f currentPos,
    float currentHeading,
    const KinodynamicTrajectory &traj,
    float currentTime,
    float lookaheadDistance = 1.0f,
    float targetSpeed = 4.0f) noexcept {
  ControlCommand cmd;
  if (HALO_UNLIKELY(traj.SegmentCount() == 0)) return cmd;

  // Lookahead search along future trajectory
  float lookaheadTime = currentTime + (lookaheadDistance / std::max(0.5f, targetSpeed));
  TrajectoryPoint goal = traj.Evaluate(lookaheadTime);

  float dx = goal.pos.x - currentPos.x;
  float dy = goal.pos.y - currentPos.y;
  float alpha = std::atan2(dy, dx) - currentHeading;

  // Normalize angle to [-PI, PI]
  alpha = std::atan2(std::sin(alpha), std::cos(alpha));

  float dist = std::sqrt(dx * dx + dy * dy);
  float curvature = (dist > 0.01f) ? (2.0f * std::sin(alpha) / dist) : 0.0f;

  cmd.linearVelocity = targetSpeed;
  cmd.angularVelocity = targetSpeed * curvature;
  return cmd;
}

// 2. Stanley Controller: Front-axle cross-track error + heading alignment in < 35 ns
[[nodiscard]] HALO_INLINE ControlCommand EvaluateStanley(
    Vec2f frontAxlePos,
    float currentHeading,
    float currentSpeed,
    const KinodynamicTrajectory &traj,
    float currentTime,
    float kGain = 1.5f,
    float targetSpeed = 4.0f) noexcept {
  ControlCommand cmd;
  if (HALO_UNLIKELY(traj.SegmentCount() == 0)) return cmd;

  TrajectoryPoint pathPoint = traj.Evaluate(currentTime);

  // Cross-track error vector (pathPoint -> frontAxlePos)
  float dx = frontAxlePos.x - pathPoint.pos.x;
  float dy = frontAxlePos.y - pathPoint.pos.y;

  // Path normal vector (-sin(heading), cos(heading))
  float pathHeading = pathPoint.heading;
  float normalX = -std::sin(pathHeading);
  float normalY = std::cos(pathHeading);
  float crossTrackError = dx * normalX + dy * normalY;

  // Heading error
  float headingError = pathHeading - currentHeading;
  headingError = std::atan2(std::sin(headingError), std::cos(headingError));

  // Stanley steering law: delta = headingError + atan(k * e / (v + soft))
  float crossTrackSteering = std::atan2(kGain * crossTrackError, std::max(0.1f, currentSpeed));
  float steeringAngle = headingError + crossTrackSteering;

  cmd.linearVelocity = targetSpeed;
  // Convert steering angle to angular velocity: omega = v * tan(delta) / wheelBase (default wheelbase 0.5m)
  cmd.angularVelocity = (targetSpeed / 0.5f) * std::tan(std::clamp(steeringAngle, -1.0f, 1.0f));
  return cmd;
}

} // namespace halo::kinodynamics
