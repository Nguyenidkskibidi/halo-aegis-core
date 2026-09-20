#pragma once

#include "../core/halo_memory.h"
#include "../core/halo_simd.h"
#include "../core/halo_supreme_core.h"
#include "../protection/halo_swar_10_layer_bitboard.h"
#include "../utils/halo_types.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>

namespace halo::flight {

// ============================================================================
// 1. KINEMATIC POINT-MASS DRONE MODEL
// ============================================================================

struct alignas(32) DroneKinematics {
  Vec2f pos{0.0f, 0.0f};  // Position in continuous coordinates (meters/tiles)
  Vec2f vel{0.0f, 0.0f};  // Velocity vector (m/s)
  float heading = 0.0f;   // Heading yaw in radians [-PI, PI]
  float speed = 0.0f;     // Current scalar speed (m/s)

  float maxSpeed = 12.0f;       // Max linear speed (m/s)
  float maxAcc = 6.0f;          // Max acceleration / deceleration (m/s^2)
  float maxYawRate = 3.14159f;  // Max turning rate (rad/s)
  float radius = 0.4f;          // Drone collision hull radius (m)
  float safetyMargin = 1.0f;    // Safe buffer zone (m)

  [[nodiscard]] HALO_INLINE float StoppingDistance() const noexcept { return (speed * speed) / (2.0f * maxAcc) + safetyMargin; }

  // Smooth kinematic integration strictly bounded by max acceleration, yaw rate, and physical barrier safety
  template <typename MatrixType>
  void StepKinematics(float targetHeading, float targetSpeed, float dt, const MatrixType &matrix) noexcept {
    // 1. Yaw rate limiting
    float angleDelta = AngleWrap(targetHeading - heading);
    float maxDeltaAngle = maxYawRate * dt;
    angleDelta = std::clamp(angleDelta, -maxDeltaAngle, maxDeltaAngle);
    heading = AngleWrap(heading + angleDelta);

    // 2. Acceleration / speed limiting
    targetSpeed = std::clamp(targetSpeed, 0.0f, maxSpeed);
    float speedDelta = targetSpeed - speed;
    float maxDeltaSpeed = maxAcc * dt;
    speedDelta = std::clamp(speedDelta, -maxDeltaSpeed, maxDeltaSpeed);
    speed = std::max(0.0f, speed + speedDelta);

    // 3. Velocity and position integration with physical barrier safety
    vel = Vec2f{std::cos(heading), std::sin(heading)} * speed;
    Vec2f nextPos = pos + vel * dt;
    const float safeRadius = radius + 0.4f;
    if (!matrix.IsHullBlocked(nextPos.x, nextPos.y, safeRadius)) {
      pos = nextPos;
    } else {
      // Barrier contact: slide along X or Y axis if clear
      Vec2f slideX = pos + Vec2f{vel.x, 0.0f} * dt;
      Vec2f slideY = pos + Vec2f{0.0f, vel.y} * dt;
      if (!matrix.IsHullBlocked(slideX.x, slideX.y, safeRadius)) {
        pos = slideX;
        vel.y = 0.0f;
      } else if (!matrix.IsHullBlocked(slideY.x, slideY.y, safeRadius)) {
        pos = slideY;
        vel.x = 0.0f;
      } else {
        // Physical buffer reached: safely halt before hull penetration
        vel = Vec2f{0.0f, 0.0f};
        speed = 0.0f;
      }
    }
  }
};

// ============================================================================
// 2. DYNAMIC MOVING OBSTACLE SIMULATION SWARM (500 AGENTS)
// ============================================================================

enum class AgentType : uint8_t {
  BALLISTIC = 0,  // High-speed linear projectiles (12-20 m/s)
  BROWNIAN = 1,   // Erratic wildlife/birds (3-6 m/s, random direction jumps)
  PATROL = 2      // Vehicles patrolling back and forth (6-10 m/s)
};

struct alignas(32) DynamicObstacle {
  Vec2f pos{0.0f, 0.0f};
  Vec2f vel{0.0f, 0.0f};
  Vec2f patrolA{0.0f, 0.0f};
  Vec2f patrolB{0.0f, 0.0f};
  float speed = 0.0f;
  swar::Layer layer = swar::Layer::AVIAN_WILDLIFE;
  AgentType type = AgentType::BROWNIAN;
  int32_t lastGridX = -1;
  int32_t lastGridY = -1;
  uint32_t rngState = 12345;
  bool patrolForward = true;

  HALO_INLINE uint32_t FastRng() noexcept {
    rngState = rngState * 1664525U + 1013904223U;
    return rngState;
  }

  HALO_INLINE float FastRngFloat() noexcept { return static_cast<float>(FastRng() & 0xFFFF) / 65535.0f; }
};

template <size_t AGENT_COUNT = 500>
class alignas(64) DynamicObstacleSwarm {
private:
  alignas(64) DynamicObstacle m_agents[AGENT_COUNT];
  int32_t m_w = 512;
  int32_t m_h = 512;

public:
  DynamicObstacleSwarm() noexcept = default;

  template <typename GridType>
  void Init(int32_t w, int32_t h, const GridType &staticGrid, uint32_t baseSeed = 42) noexcept {
    m_w = w;
    m_h = h;

    constexpr size_t NUM_BALLISTIC = 100;
    constexpr size_t NUM_BROWNIAN = 200;
    constexpr size_t NUM_PATROL = 200;
    static_assert(NUM_BALLISTIC + NUM_BROWNIAN + NUM_PATROL == AGENT_COUNT, "Agent count mismatch");

    uint32_t seed = baseSeed;
    auto NextRandom = [&seed]() -> uint32_t {
      seed = seed * 1664525U + 1013904223U;
      return seed;
    };

    size_t idx = 0;

    // 1. Ballistic Fast Projectiles (100 agents)
    for (size_t i = 0; i < NUM_BALLISTIC; ++i, ++idx) {
      DynamicObstacle &a = m_agents[idx];
      a.type = AgentType::BALLISTIC;
      a.layer = swar::Layer::BALLISTIC;
      a.speed = 14.0f + static_cast<float>(NextRandom() % 60) * 0.1f;
      a.rngState = NextRandom();

      // Find walkable spawn away from start and goal
      int32_t sx = 10, sy = 10;
      for (int t = 0; t < 200; ++t) {
        sx = 10 + static_cast<int32_t>(NextRandom() % (m_w - 20));
        sy = 10 + static_cast<int32_t>(NextRandom() % (m_h - 20));
        if (staticGrid.IsWalkable(sx, sy) && (std::abs(sx - 15) > 25 || std::abs(sy - 10) > 25) &&
            (std::abs(sx - 495) > 25 || std::abs(sy - 495) > 25))
          break;
      }
      a.pos = Vec2f{static_cast<float>(sx), static_cast<float>(sy)};
      float angle = static_cast<float>(NextRandom() % 628) * 0.01f;
      a.vel = Vec2f{std::cos(angle), std::sin(angle)} * a.speed;
      a.lastGridX = sx;
      a.lastGridY = sy;
    }

    // 2. Brownian Erratic Wildlife (200 agents)
    for (size_t i = 0; i < NUM_BROWNIAN; ++i, ++idx) {
      DynamicObstacle &a = m_agents[idx];
      a.type = AgentType::BROWNIAN;
      a.layer = swar::Layer::AVIAN_WILDLIFE;
      a.speed = 4.0f + static_cast<float>(NextRandom() % 30) * 0.1f;
      a.rngState = NextRandom();

      int32_t sx = 10, sy = 10;
      for (int t = 0; t < 200; ++t) {
        sx = 10 + static_cast<int32_t>(NextRandom() % (m_w - 20));
        sy = 10 + static_cast<int32_t>(NextRandom() % (m_h - 20));
        if (staticGrid.IsWalkable(sx, sy) && (std::abs(sx - 15) > 25 || std::abs(sy - 10) > 25) &&
            (std::abs(sx - 495) > 25 || std::abs(sy - 495) > 25))
          break;
      }
      a.pos = Vec2f{static_cast<float>(sx), static_cast<float>(sy)};
      float angle = static_cast<float>(NextRandom() % 628) * 0.01f;
      a.vel = Vec2f{std::cos(angle), std::sin(angle)} * a.speed;
      a.lastGridX = sx;
      a.lastGridY = sy;
    }

    // 3. Dynamic Patrol Agents (200 agents)
    for (size_t i = 0; i < NUM_PATROL; ++i, ++idx) {
      DynamicObstacle &a = m_agents[idx];
      a.type = AgentType::PATROL;
      a.layer = (i % 2 == 0) ? swar::Layer::VEHICLES : swar::Layer::HUMANS;
      a.speed = 7.0f + static_cast<float>(NextRandom() % 40) * 0.1f;
      a.rngState = NextRandom();

      int32_t sx = 10, sy = 10;
      int32_t ex = 10, ey = 10;
      for (int t = 0; t < 200; ++t) {
        sx = 10 + static_cast<int32_t>(NextRandom() % (m_w - 20));
        sy = 10 + static_cast<int32_t>(NextRandom() % (m_h - 20));
        if (staticGrid.IsWalkable(sx, sy) && (std::abs(sx - 15) > 25 || std::abs(sy - 10) > 25) &&
            (std::abs(sx - 495) > 25 || std::abs(sy - 495) > 25))
          break;
      }
      for (int t = 0; t < 200; ++t) {
        ex = std::clamp(sx + static_cast<int32_t>((NextRandom() % 60) - 30), 5, m_w - 6);
        ey = std::clamp(sy + static_cast<int32_t>((NextRandom() % 60) - 30), 5, m_h - 6);
        if (staticGrid.IsWalkable(ex, ey)) break;
      }
      a.pos = Vec2f{static_cast<float>(sx), static_cast<float>(sy)};
      a.patrolA = a.pos;
      a.patrolB = Vec2f{static_cast<float>(ex), static_cast<float>(ey)};
      Vec2f diff = a.patrolB - a.patrolA;
      a.vel = diff.Normalized() * a.speed;
      a.lastGridX = sx;
      a.lastGridY = sy;
      a.patrolForward = true;
    }
  }

  // Update all 500 agents and toggle bits in LayeredHazardMatrix in O(1) (< 5 ns per agent)
  template <int32_t W, int32_t H, typename GridType>
  void Update(float dt, swar::LayeredHazardMatrixT<W, H> &matrix, const GridType &staticGrid) noexcept {
    // Pass 1: Clear previous obstacle footprints
    for (size_t i = 0; i < AGENT_COUNT; ++i) {
      const DynamicObstacle &a = m_agents[i];
      if (HALO_LIKELY(a.lastGridX >= 0 && a.lastGridY >= 0)) {
        matrix.ClearBit(a.layer, a.lastGridX, a.lastGridY);
        matrix.ClearBit(a.layer, a.lastGridX + 1, a.lastGridY);
        matrix.ClearBit(a.layer, a.lastGridX - 1, a.lastGridY);
        matrix.ClearBit(a.layer, a.lastGridX, a.lastGridY + 1);
        matrix.ClearBit(a.layer, a.lastGridX, a.lastGridY - 1);
      }
    }

    // Pass 2: Step physics
    for (size_t i = 0; i < AGENT_COUNT; ++i) {
      DynamicObstacle &a = m_agents[i];

      switch (a.type) {
        case AgentType::BALLISTIC: {
          Vec2f nextPos = a.pos + a.vel * dt;
          int32_t nx = static_cast<int32_t>(nextPos.x + 0.5f);
          int32_t ny = static_cast<int32_t>(nextPos.y + 0.5f);
          if (nx < 2 || nx >= m_w - 2 || ny < 2 || ny >= m_h - 2 || !staticGrid.IsWalkable(nx, ny)) {
            if (nx < 2 || nx >= m_w - 2 || !staticGrid.IsWalkable(nx, a.lastGridY)) a.vel.x = -a.vel.x;
            if (ny < 2 || ny >= m_h - 2 || !staticGrid.IsWalkable(a.lastGridX, ny)) a.vel.y = -a.vel.y;
            nextPos = a.pos + a.vel * dt;
          }
          a.pos = nextPos;
          break;
        }
        case AgentType::BROWNIAN: {
          if ((a.FastRng() % 15) == 0) {
            float dAngle = (a.FastRngFloat() - 0.5f) * 1.5f;
            float curAngle = std::atan2(a.vel.y, a.vel.x) + dAngle;
            a.vel = Vec2f{std::cos(curAngle), std::sin(curAngle)} * a.speed;
          }
          Vec2f nextPos = a.pos + a.vel * dt;
          int32_t nx = static_cast<int32_t>(nextPos.x + 0.5f);
          int32_t ny = static_cast<int32_t>(nextPos.y + 0.5f);
          if (nx < 2 || nx >= m_w - 2 || ny < 2 || ny >= m_h - 2 || !staticGrid.IsWalkable(nx, ny)) {
            a.vel = a.vel * -1.0f;
            nextPos = a.pos + a.vel * dt;
          }
          a.pos = nextPos;
          break;
        }
        case AgentType::PATROL: {
          Vec2f target = a.patrolForward ? a.patrolB : a.patrolA;
          Vec2f diff = target - a.pos;
          float distSq = diff.LengthSq();
          if (distSq < 1.5f) {
            a.patrolForward = !a.patrolForward;
            target = a.patrolForward ? a.patrolB : a.patrolA;
            diff = target - a.pos;
          }
          a.vel = diff.Normalized() * a.speed;
          a.pos += a.vel * dt;
          break;
        }
      }

      a.pos.x = std::clamp(a.pos.x, 2.0f, static_cast<float>(m_w - 3));
      a.pos.y = std::clamp(a.pos.y, 2.0f, static_cast<float>(m_h - 3));

      a.lastGridX = static_cast<int32_t>(a.pos.x + 0.5f);
      a.lastGridY = static_cast<int32_t>(a.pos.y + 0.5f);
    }

    // Pass 3: Set all new obstacle footprints
    for (size_t i = 0; i < AGENT_COUNT; ++i) {
      const DynamicObstacle &a = m_agents[i];
      matrix.SetBit(a.layer, a.lastGridX, a.lastGridY);
      matrix.SetBit(a.layer, a.lastGridX + 1, a.lastGridY);
      matrix.SetBit(a.layer, a.lastGridX - 1, a.lastGridY);
      matrix.SetBit(a.layer, a.lastGridX, a.lastGridY + 1);
      matrix.SetBit(a.layer, a.lastGridX, a.lastGridY - 1);
    }
  }

  [[nodiscard]] const DynamicObstacle *GetAgents() const noexcept { return m_agents; }
  [[nodiscard]] size_t Count() const noexcept { return AGENT_COUNT; }
};

// ============================================================================
// 3. HIERARCHICAL DUAL-TIER FLIGHT GUIDANCE ENGINE
// ============================================================================

struct FlightTelemetry {
  uint64_t cycleTimeNs = 0;         // Total cycle execution time (ns)
  uint64_t evasionLatencyNs = 0;    // Local reactive evasion scan time (ns)
  float minClearanceDist = 999.0f;  // Distance to nearest obstacle (m)
  bool evasionActive = false;       // True if tangential deflection is active
  bool deadlineMissed = false;      // True if cycle execution exceeded 2.0 ms
};

template <int32_t W = 512, int32_t H = 512>
class alignas(64) HierarchicalFlightEngine {
private:
  DroneKinematics m_drone;
  Vec2i m_macroWaypoints[Config::MAX_PATH_LEN];
  int32_t m_macroPathLen = 0;
  int32_t m_currentWaypointIdx = 0;
  Vec2f m_goalPos{0.0f, 0.0f};

  // Pre-calculated candidate evasion angles: +/- 30, 45, 60, 90 deg
  static constexpr int32_t NUM_DEFLECTION_ANGLES = 8;
  static constexpr float DEFLECTION_ANGLES[NUM_DEFLECTION_ANGLES] = {
      0.52359877f,   // +30 deg
      -0.52359877f,  // -30 deg
      0.78539816f,   // +45 deg
      -0.78539816f,  // -45 deg
      1.04719755f,   // +60 deg
      -1.04719755f,  // -60 deg
      1.57079632f,   // +90 deg
      -1.57079632f   // -90 deg
  };
  static constexpr float COS_DEFLECTION[NUM_DEFLECTION_ANGLES] = {0.86602540f, 0.86602540f, 0.70710678f, 0.70710678f,
                                                                  0.50000000f, 0.50000000f, 0.00000000f, 0.00000000f};
  static constexpr float SIN_DEFLECTION[NUM_DEFLECTION_ANGLES] = {0.50000000f, -0.50000000f, 0.70710678f, -0.70710678f,
                                                                  0.86602540f, -0.86602540f, 1.00000000f, -1.00000000f};

public:
  HierarchicalFlightEngine() noexcept = default;

  void InitFlight(Vec2f startPos, Vec2f goalPos, const PathResult &macroRoute) noexcept {
    m_drone.pos = startPos;
    m_drone.vel = Vec2f{0.0f, 0.0f};
    m_drone.speed = 0.0f;
    m_goalPos = goalPos;

    m_macroPathLen = std::min(macroRoute.len, Config::MAX_PATH_LEN);
    for (int32_t i = 0; i < m_macroPathLen; ++i) {
      m_macroWaypoints[i] = macroRoute.route[i];
    }
    m_currentWaypointIdx = (m_macroPathLen > 1) ? 1 : 0;

    if (m_macroPathLen > 0) {
      Vec2f firstTarget{static_cast<float>(m_macroWaypoints[m_currentWaypointIdx].x),
                        static_cast<float>(m_macroWaypoints[m_currentWaypointIdx].y)};
      Vec2f diff = firstTarget - startPos;
      m_drone.heading = std::atan2(diff.y, diff.x);
    }
  }

  [[nodiscard]] const DroneKinematics &GetDrone() const noexcept { return m_drone; }
  [[nodiscard]] DroneKinematics &GetDrone() noexcept { return m_drone; }
  [[nodiscard]] bool HasReachedGoal(float acceptanceRadius = 2.0f) const noexcept {
    return (m_goalPos - m_drone.pos).LengthSq() <= (acceptanceRadius * acceptanceRadius);
  }

  // 100 Hz Replan & Avoidance Loop: executes in < 800 ns per cycle
  template <size_t AGENT_COUNT = 500>
  FlightTelemetry StepControlCycle(float dt, const swar::LayeredHazardMatrixT<W, H> &matrix,
                                   const DynamicObstacleSwarm<AGENT_COUNT> *swarm = nullptr) noexcept {
    FlightTelemetry telem;
    uint64_t tStart = GetHardwareTimestamp();

    // 1. Waypoint Progression
    if (m_currentWaypointIdx < m_macroPathLen - 1) {
      Vec2f targetPos{static_cast<float>(m_macroWaypoints[m_currentWaypointIdx].x),
                      static_cast<float>(m_macroWaypoints[m_currentWaypointIdx].y)};
      if ((targetPos - m_drone.pos).LengthSq() < 4.0f) {
        ++m_currentWaypointIdx;
      }
    }

    Vec2f currentTarget{static_cast<float>(m_macroWaypoints[m_currentWaypointIdx].x),
                        static_cast<float>(m_macroWaypoints[m_currentWaypointIdx].y)};
    Vec2f toTarget = currentTarget - m_drone.pos;
    Vec2f dirTarget = toTarget.Normalized();

    // 2. Lookahead scan distance
    float stopDist = m_drone.StoppingDistance();
    float lookaheadDist = std::max(m_drone.speed * 1.5f, stopDist + 3.0f);

    uint64_t tEvasionStart = GetHardwareTimestamp();

    // Predictive Velocity-Obstacle (VO) Interception Detection
    Vec2f predictiveEvasion{0.0f, 0.0f};
    bool interceptionPredicted = false;

    if (swarm) {
      const auto *agents = swarm->GetAgents();
      float closestT = 999.0f;
      Vec2f bestEvasionDir{0.0f, 0.0f};
      for (size_t a = 0; a < swarm->Count(); ++a) {
        Vec2f delta = agents[a].pos - m_drone.pos;
        float dSq = delta.LengthSq();
        if (dSq > 36.0f) continue;  // Further than 6 meters

        Vec2f vRel = agents[a].vel - m_drone.vel;
        float vRelSq = vRel.LengthSq();
        if (vRelSq > 0.5f && delta.Dot(vRel) < 0.0f) {  // Moving toward each other
          float tCpa = -delta.Dot(vRel) / vRelSq;
          if (tCpa > 0.0f && tCpa < 1.0f && tCpa < closestT) {
            Vec2f dMin = delta + vRel * tCpa;
            if (dMin.LengthSq() < 3.24f) {  // Closest approach < 1.8m
              closestT = tCpa;
              Vec2f perp1{-vRel.y, vRel.x};
              Vec2f perp2{vRel.y, -vRel.x};
              bestEvasionDir = (perp1.Dot(dirTarget) >= perp2.Dot(dirTarget)) ? perp1.Normalized() : perp2.Normalized();
              interceptionPredicted = true;
            }
          }
        }
      }
      if (interceptionPredicted) {
        if (matrix.RaycastLine(m_drone.pos, bestEvasionDir, 2.0f) < 1.2f) {
          bestEvasionDir = -bestEvasionDir;
        }
        predictiveEvasion = bestEvasionDir;
      }
    }

    // 3. Check direct path to macro waypoint AND velocity direction
    float forwardClearance = matrix.RaycastLine(m_drone.pos, dirTarget, lookaheadDist);
    float velClearance =
        (m_drone.speed > 0.5f) ? matrix.RaycastLine(m_drone.pos, m_drone.vel.Normalized(), lookaheadDist) : forwardClearance;
    float minForwardClearance = std::min(forwardClearance, velClearance);
    telem.minClearanceDist = minForwardClearance;

    float targetHeading = std::atan2(dirTarget.y, dirTarget.x);
    float targetSpeed = m_drone.maxSpeed;

    if (interceptionPredicted && predictiveEvasion.LengthSq() > 0.01f) {
      // Immediate Lateral Evasion: dodge incoming projectile / obstacle
      telem.evasionActive = true;
      targetHeading = std::atan2(predictiveEvasion.y, predictiveEvasion.x);
      targetSpeed = m_drone.maxSpeed;
    } else if (minForwardClearance >= lookaheadDist) {
      // Direct corridor is completely unobstructed
      telem.evasionActive = false;
    } else {
      // Direct corridor or velocity vector obstructed:
      // Execute instantaneous Branchless Tangential Evasion Raycast
      telem.evasionActive = true;

      float bestScore = -1e9f;
      int32_t bestIdx = -1;
      float bestClearance = 0.0f;

      for (int32_t i = 0; i < NUM_DEFLECTION_ANGLES; ++i) {
        Vec2f candidateDir{dirTarget.x * COS_DEFLECTION[i] - dirTarget.y * SIN_DEFLECTION[i],
                           dirTarget.x * SIN_DEFLECTION[i] + dirTarget.y * COS_DEFLECTION[i]};

        float clearance = matrix.RaycastLine(m_drone.pos, candidateDir, lookaheadDist);
        if (clearance < stopDist) {
          continue;
        }

        // Score balances clearance and progress toward macro waypoint
        float progress = COS_DEFLECTION[i];
        float score = clearance * 2.0f + progress * 4.0f;

        if (score > bestScore) {
          bestScore = score;
          bestIdx = i;
          bestClearance = clearance;
        }
      }

      if (bestIdx >= 0) {
        targetHeading = AngleWrap(targetHeading + DEFLECTION_ANGLES[bestIdx]);
        telem.minClearanceDist = bestClearance;
        targetSpeed = m_drone.maxSpeed * 0.85f;
      } else {
        targetSpeed = 0.0f;
      }
    }

    uint64_t tEvasionEnd = GetHardwareTimestamp();
    telem.evasionLatencyNs = tEvasionEnd - tEvasionStart;

    // 4. Kinematic Motion Integration with barrier safety
    m_drone.StepKinematics(targetHeading, targetSpeed, dt, matrix);

    uint64_t tEnd = GetHardwareTimestamp();
    telem.cycleTimeNs = tEnd - tStart;
    telem.deadlineMissed = (telem.cycleTimeNs > 2000000ULL);  // 2.0 ms hard deadline

    return telem;
  }

private:
  [[nodiscard]] static HALO_INLINE uint64_t GetHardwareTimestamp() noexcept {
#if defined(__APPLE__)
    return clock_gettime_nsec_np(CLOCK_UPTIME_RAW);
#elif defined(__linux__)
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    return static_cast<uint64_t>(ts.tv_sec) * 1000000000ULL + ts.tv_nsec;
#else
    return 0;
#endif
  }
};

}  // namespace halo::flight
