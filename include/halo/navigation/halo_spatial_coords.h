#pragma once

#include <cmath>
#include <cstdint>

namespace halo::spatial {

// ============================================================================
// WGS84 ELLIPSOIDAL CONSTANTS & GEODETIC MODELS
// ============================================================================

struct WGS84Constants {
  static constexpr double SEMI_MAJOR_AXIS_A = 6378137.0;  // meters
  static constexpr double FLATTENING_F = 1.0 / 298.257223563;
  static constexpr double SEMI_MINOR_AXIS_B = SEMI_MAJOR_AXIS_A * (1.0 - FLATTENING_F);
  static constexpr double ECCENTRICITY_SQ_E2 = FLATTENING_F * (2.0 - FLATTENING_F);
  static constexpr double DEG_TO_RAD = 0.017453292519943295769236907684886;
  static constexpr double RAD_TO_DEG = 57.295779513082320876798154814105;
};

// WGS84 Geodetic Position (Degrees, Meters)
struct GeodeticCoord {
  double lat = 0.0;  // Latitude in decimal degrees [-90.0, +90.0]
  double lon = 0.0;  // Longitude in decimal degrees [-180.0, +180.0]
  double alt = 0.0;  // Height above WGS84 ellipsoid in meters

  constexpr GeodeticCoord() noexcept = default;
  constexpr GeodeticCoord(double _lat, double _lon, double _alt = 0.0) noexcept : lat(_lat), lon(_lon), alt(_alt) {}
};

// High-Precision Metric Offset in Local Tangent Plane (East-North-Up)
struct MetricCoord3D {
  double x = 0.0;  // East in meters
  double y = 0.0;  // North in meters
  double z = 0.0;  // Up in meters

  constexpr MetricCoord3D() noexcept = default;
  constexpr MetricCoord3D(double _x, double _y, double _z = 0.0) noexcept : x(_x), y(_y), z(_z) {}

  [[nodiscard]] double Distance2D(const MetricCoord3D &o) const noexcept {
    double dx = x - o.x;
    double dy = y - o.y;
    return std::sqrt(dx * dx + dy * dy);
  }

  [[nodiscard]] double Distance3D(const MetricCoord3D &o) const noexcept {
    double dx = x - o.x;
    double dy = y - o.y;
    double dz = z - o.z;
    return std::sqrt(dx * dx + dy * dy + dz * dz);
  }
};

// Parametric 2D Metric Spatial Bounding Box
struct SpatialExtent2D {
  double minX = 0.0;
  double minY = 0.0;
  double maxX = 0.0;
  double maxY = 0.0;

  constexpr SpatialExtent2D() noexcept = default;
  constexpr SpatialExtent2D(double _minX, double _minY, double _maxX, double _maxY) noexcept
      : minX(_minX), minY(_minY), maxX(_maxX), maxY(_maxY) {}

  [[nodiscard]] constexpr double Width() const noexcept { return maxX - minX; }
  [[nodiscard]] constexpr double Height() const noexcept { return maxY - minY; }

  [[nodiscard]] constexpr bool Contains(double x, double y) const noexcept { return x >= minX && x <= maxX && y >= minY && y <= maxY; }

  [[nodiscard]] constexpr bool Contains(const MetricCoord3D &pt) const noexcept { return Contains(pt.x, pt.y); }

  void Expand(double margin) noexcept {
    minX -= margin;
    minY -= margin;
    maxX += margin;
    maxY += margin;
  }
};

// ============================================================================
// UNIVERSAL LOCAL TANGENT PLANE (ENU) PROJECTION ADAPTER
// ============================================================================

class LocalTangentPlane {
private:
  GeodeticCoord m_datum;
  double m_refLatRad = 0.0;
  double m_refLonRad = 0.0;
  double m_sinLat = 0.0;
  double m_cosLat = 0.0;
  double m_sinLon = 0.0;
  double m_cosLon = 0.0;

  // Curvature radii at reference latitude
  double m_radiusNorth = 0.0;  // Meridional radius (M)
  double m_radiusEast = 0.0;   // Normal radius (N)

  // Precomputed Earth-Centered Earth-Fixed (ECEF) anchor point
  double m_refECEF_X = 0.0;
  double m_refECEF_Y = 0.0;
  double m_refECEF_Z = 0.0;

public:
  constexpr LocalTangentPlane() noexcept = default;

  explicit LocalTangentPlane(const GeodeticCoord &datumAnchor) noexcept { SetDatum(datumAnchor); }

  void SetDatum(const GeodeticCoord &datumAnchor) noexcept {
    m_datum = datumAnchor;
    m_refLatRad = datumAnchor.lat * WGS84Constants::DEG_TO_RAD;
    m_refLonRad = datumAnchor.lon * WGS84Constants::DEG_TO_RAD;

    m_sinLat = std::sin(m_refLatRad);
    m_cosLat = std::cos(m_refLatRad);
    m_sinLon = std::sin(m_refLonRad);
    m_cosLon = std::cos(m_refLonRad);

    double sinSqLat = m_sinLat * m_sinLat;
    double denom = std::sqrt(1.0 - WGS84Constants::ECCENTRICITY_SQ_E2 * sinSqLat);

    m_radiusEast = WGS84Constants::SEMI_MAJOR_AXIS_A / denom;
    m_radiusNorth = (WGS84Constants::SEMI_MAJOR_AXIS_A * (1.0 - WGS84Constants::ECCENTRICITY_SQ_E2)) / (denom * denom * denom);

    // Compute reference ECEF coordinates
    m_refECEF_X = (m_radiusEast + m_datum.alt) * m_cosLat * m_cosLon;
    m_refECEF_Y = (m_radiusEast + m_datum.alt) * m_cosLat * m_sinLon;
    m_refECEF_Z = (m_radiusEast * (1.0 - WGS84Constants::ECCENTRICITY_SQ_E2) + m_datum.alt) * m_sinLat;
  }

  [[nodiscard]] const GeodeticCoord &GetDatum() const noexcept { return m_datum; }

  // Convert Geodetic (WGS84) to Local Tangent Plane (ENU meters)
  [[nodiscard]] MetricCoord3D GeodeticToENU(const GeodeticCoord &geo) const noexcept {
    double latRad = geo.lat * WGS84Constants::DEG_TO_RAD;
    double lonRad = geo.lon * WGS84Constants::DEG_TO_RAD;

    double sinLat = std::sin(latRad);
    double cosLat = std::cos(latRad);
    double sinLon = std::sin(lonRad);
    double cosLon = std::cos(lonRad);

    double denom = std::sqrt(1.0 - WGS84Constants::ECCENTRICITY_SQ_E2 * sinLat * sinLat);
    double N = WGS84Constants::SEMI_MAJOR_AXIS_A / denom;

    double ecefX = (N + geo.alt) * cosLat * cosLon;
    double ecefY = (N + geo.alt) * cosLat * sinLon;
    double ecefZ = (N * (1.0 - WGS84Constants::ECCENTRICITY_SQ_E2) + geo.alt) * sinLat;

    double dx = ecefX - m_refECEF_X;
    double dy = ecefY - m_refECEF_Y;
    double dz = ecefZ - m_refECEF_Z;

    // Standard ECEF to ENU rotation matrix
    double east = -m_sinLon * dx + m_cosLon * dy;
    double north = -m_sinLat * m_cosLon * dx - m_sinLat * m_sinLon * dy + m_cosLat * dz;
    double up = m_cosLat * m_cosLon * dx + m_cosLat * m_sinLon * dy + m_sinLat * dz;

    return MetricCoord3D(east, north, up);
  }

  // Fast Flat-Earth approximation for local regions (< 50 km) in < 3 ns
  [[nodiscard]] MetricCoord3D FastGeodeticToENU(const GeodeticCoord &geo) const noexcept {
    double dLatRad = (geo.lat - m_datum.lat) * WGS84Constants::DEG_TO_RAD;
    double dLonRad = (geo.lon - m_datum.lon) * WGS84Constants::DEG_TO_RAD;

    double east = dLonRad * (m_radiusEast + m_datum.alt) * m_cosLat;
    double north = dLatRad * (m_radiusNorth + m_datum.alt);
    double up = geo.alt - m_datum.alt;

    return MetricCoord3D(east, north, up);
  }

  // Convert Local Tangent Plane (ENU meters) to Geodetic (WGS84)
  [[nodiscard]] GeodeticCoord ENUToGeodetic(const MetricCoord3D &enu) const noexcept {
    // ENU to ECEF rotation
    double dx = -m_sinLon * enu.x - m_sinLat * m_cosLon * enu.y + m_cosLat * m_cosLon * enu.z;
    double dy = m_cosLon * enu.x - m_sinLat * m_sinLon * enu.y + m_cosLat * m_sinLon * enu.z;
    double dz = m_cosLat * enu.y + m_sinLat * enu.z;

    double x = m_refECEF_X + dx;
    double y = m_refECEF_Y + dy;
    double z = m_refECEF_Z + dz;

    // Bowring's closed-form ECEF to Geodetic algorithm
    double p = std::sqrt(x * x + y * y);
    double theta = std::atan2(z * WGS84Constants::SEMI_MAJOR_AXIS_A, p * WGS84Constants::SEMI_MINOR_AXIS_B);

    double sinTheta = std::sin(theta);
    double cosTheta = std::cos(theta);

    double ePrimeSq = (WGS84Constants::SEMI_MAJOR_AXIS_A * WGS84Constants::SEMI_MAJOR_AXIS_A -
                       WGS84Constants::SEMI_MINOR_AXIS_B * WGS84Constants::SEMI_MINOR_AXIS_B) /
                      (WGS84Constants::SEMI_MINOR_AXIS_B * WGS84Constants::SEMI_MINOR_AXIS_B);

    double lat = std::atan2(z + ePrimeSq * WGS84Constants::SEMI_MINOR_AXIS_B * sinTheta * sinTheta * sinTheta,
                            p - WGS84Constants::ECCENTRICITY_SQ_E2 * WGS84Constants::SEMI_MAJOR_AXIS_A * cosTheta * cosTheta * cosTheta);
    double lon = std::atan2(y, x);

    double sinLat = std::sin(lat);
    double N = WGS84Constants::SEMI_MAJOR_AXIS_A / std::sqrt(1.0 - WGS84Constants::ECCENTRICITY_SQ_E2 * sinLat * sinLat);
    double alt = p / std::cos(lat) - N;

    return GeodeticCoord(lat * WGS84Constants::RAD_TO_DEG, lon * WGS84Constants::RAD_TO_DEG, alt);
  }
};

}  // namespace halo::spatial
