#pragma once

#include <cmath>

namespace kart {

constexpr float PI = 3.14159265f;
constexpr float DT = 1.f / 60.f;
constexpr float STRAIGHT = 180.f;
constexpr float RADIUS = 64.f;
constexpr float TURN = PI * RADIUS;
constexpr float LAP = 2.f * STRAIGHT + 2.f * TURN;
constexpr int LAPS = 8;

constexpr float HALF_W = 12.f;
constexpr float BERM = 5.f;
constexpr float LAT = 16.f;
constexpr float CENT_K = 0.32f;
constexpr float MAX_V = 52.f;
constexpr float GRASS_V = 31.f;
constexpr float ACCEL = 48.f;
constexpr float BRAKE = 72.f;
constexpr float COAST = 16.f;

constexpr float FOCAL = 210.f;
constexpr float CAM_BACK = 13.f;
constexpr float CAM_H = 5.2f;
constexpr int HORIZON = 74;
constexpr float SPR_H = 3.3f;

constexpr float MAP_S = 0.20f;
constexpr int MAP_W = 80;
constexpr int MAP_H = 48;
constexpr int MAP_CX = 40;
constexpr int MAP_CY = 24;

struct Pose {
    float x = 0, z = 0, hdg = 0;
};

inline float wrap(float s, float m) {
    s = std::fmod(s, m);
    if (s < 0) s += m;
    return s;
}

inline float wrapAngle(float a) {
    while (a > PI) a -= 2.f * PI;
    while (a < -PI) a += 2.f * PI;
    return a;
}

// Stadium oval, counterclockwise. s=0 is the start line, heading +X.
inline Pose centerline(float s) {
    float u = wrap(s, LAP);
    if (u < STRAIGHT) return {-STRAIGHT * 0.5f + u, -RADIUS, 0.f};
    u -= STRAIGHT;
    if (u < TURN) {
        float a = u / RADIUS;
        return {STRAIGHT * 0.5f + std::sin(a) * RADIUS, -std::cos(a) * RADIUS, a};
    }
    u -= TURN;
    if (u < STRAIGHT) return {STRAIGHT * 0.5f - u, RADIUS, PI};
    u -= STRAIGHT;
    float a = u / RADIUS;
    return {-STRAIGHT * 0.5f - std::sin(a) * RADIUS, std::cos(a) * RADIUS, PI + a};
}

inline Pose poseAt(float s, float n) {
    Pose p = centerline(s);
    p.x += std::sin(p.hdg) * n;
    p.z -= std::cos(p.hdg) * n;
    return p;
}

inline float curvature(float s) {
    float u = wrap(s, LAP);
    if (u < STRAIGHT) return 0.f;
    u -= STRAIGHT;
    if (u < TURN) return 1.f / RADIUS;
    u -= TURN;
    if (u < STRAIGHT) return 0.f;
    return 1.f / RADIUS;
}

// Outside before a left-hand turn, inside at the apex. Positive n is outside.
inline float racingLine(float s) {
    float u = wrap(s, LAP);
    auto straight = [](float along) {
        float remain = STRAIGHT - along;
        if (remain < 50.f) return (1.f - remain / 50.f) * 6.5f;
        if (along < 40.f) return 5.5f * (1.f - along / 40.f);
        return 0.f;
    };
    auto bend = [](float along) {
        float t = along / TURN;
        if (t < 0.22f) return 6.5f + (-8.f - 6.5f) * (t / 0.22f);
        if (t < 0.62f) return -8.f;
        return -8.f + (5.5f + 8.f) * ((t - 0.62f) / 0.38f);
    };
    if (u < STRAIGHT) return straight(u);
    u -= STRAIGHT;
    if (u < TURN) return bend(u);
    u -= TURN;
    if (u < STRAIGHT) return straight(u);
    return bend(u - STRAIGHT);
}

}  // namespace kart
