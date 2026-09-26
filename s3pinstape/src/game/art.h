// Pictures drawn at boot. Nothing is loaded from a file.
#pragma once

#include "console/gfx.h"
#include "console/vdp.h"

namespace pinstape {

// Five-pin rack, left to right in the drawer: 2, 3, 5, 3, 2.
// Index: 0 head (5), 1 left three, 2 right three, 3 left two, 4 right two.
constexpr float kPinX[5] = {0.f, -0.50f, 0.50f, -1.00f, 1.00f};
constexpr float kPinZ[5] = {1.00f, 1.50f, 1.50f, 2.00f, 2.00f};
constexpr int kWorth[5] = {5, 3, 3, 2, 2};
constexpr int kSlot[5] = {2, 1, 3, 0, 4};

// Wider than half the gap, narrower than the gap. The centre of a pin takes
// that pin alone. The point between a 2 and a 3 takes both, which sums to 5
// and is still not the head.
constexpr float kHit = 0.27f;
constexpr float kHook = 1.15f;
constexpr float kTrue = 0.12f;
constexpr float kPeriod = 1.15f;

constexpr int kRowY[3] = {23, 39, 55};
constexpr int kMarkX0 = 100;
constexpr int kMarkPitch = 11;
constexpr float kDrawerY = 184.f;

inline float laneX(float x, float z) {
    float u = (z - 0.35f) / 1.70f;
    if (u < 0.f) u = 0.f;
    if (u > 1.f) u = 1.f;
    return 160.f + x * 78.f * (1.f - u * 0.12f);
}

inline float laneY(float z) {
    float u = (z - 0.35f) / 1.70f;
    if (u < 0.f) u = 0.f;
    if (u > 1.f) u = 1.f;
    return 136.f - u * 54.f;
}

inline float drawerX(int pin) { return 48.f + float(kSlot[pin]) * 56.f; }

enum Pal {
    PAL_TEXT = 0,
    PAL_GOLD = 1,
    PAL_AMBER = 2,
    PAL_GREEN = 3,
    PAL_RED = 4,
    PAL_HOUSE = 5,
    PAL_PIN = 6,
    PAL_BALL = 7,
    PAL_BOWLER = 8,
    PAL_SHADE = 9
};

struct Art {
    int font[96] = {};
    gs::Mipped pin;
    gs::Mipped ball;
    gs::Mipped bowler[3];
    gs::Mipped arrow;
    gs::Mipped dot;
    gs::Mipped tick;
    gs::Mipped shadow;
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace pinstape
