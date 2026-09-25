#pragma once

#include <cmath>

// Overhead cul-de-sac. +lat is screen-right, +z runs up the street.
// The projection is affine so a ground sprite rasterised with it tiles.
namespace gig {

constexpr float LX = 18.f;
constexpr float LY = 3.1f;
constexpr float ZX = 6.f;
constexpr float ZY = -9.f;
constexpr float CX0 = 156.f;
constexpr float CY0 = 150.f;
constexpr float SEG = 4.f;
constexpr float DT = 1.f / 60.f;

constexpr float ROAD_HALF = 3.32f;
constexpr float WALK = 0.78f;
constexpr float LANE_LIM = 2.82f;
constexpr float HOUSE_LAT = 6.42f;
constexpr float PORCH_IN = 1.78f;
constexpr float REACH = 2.42f;
constexpr float LEAD = 1.62f;
constexpr float CATCH_R = 0.84f;
constexpr float MAIL_R = 0.40f;
constexpr float SPEED = 2.32f;
constexpr float STEER_RATE = 7.4f;
constexpr float SPACING = 11.6f;
constexpr float SHIFT_TIME = 110.f;
constexpr int TARGET = 12;
constexpr int AMMO_MAX = 5;

inline void project(float lat, float zrel, float& sx, float& sy) {
    sx = CX0 + lat * LX + zrel * ZX;
    sy = CY0 + lat * LY + zrel * ZY;
}

inline void unproject(float dx, float dy, float& lat, float& z) {
    constexpr float det = LX * ZY - ZX * LY;
    lat = (dx * ZY - ZX * dy) / det;
    z = (LX * dy - dx * LY) / det;
}

inline float hash01(int n) {
    float x = std::sin(float(n) * 12.9898f) * 43758.5453f;
    return x - std::floor(x);
}

}  // namespace gig
