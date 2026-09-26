// S3 FUNICULAR sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include <cmath>

#include "console/gfx.h"
#include "console/vdp.h"

namespace funicular {

// The whole incline is on screen. s = 0 is the valley buffer, s = kLen the bullwheel.
constexpr float kLen = 100.f;
constexpr float kX0 = 42.f;
constexpr float kY0 = 176.f;
constexpr float kX1 = 288.f;
constexpr float kY1 = 40.f;

enum Pal {
    PAL_HUD = 0,
    PAL_RED = 1,
    PAL_GREEN = 2,
    PAL_DECK = 3,
    PAL_STEEL = 4,
    PAL_FOLK = 5,
    PAL_HOUSE = 6,
    PAL_GAUGE = 7,
    PAL_HILL = 8
};

struct Art {
    gs::Image hill;
    gs::Mipped car;
    float sillX = 20.f, sillY = 23.f;
    gs::Image deck;
    float deckX = 3.f, deckY = 16.f;
    gs::Image house;
    float houseX = 24.f, houseY = 40.f;
    gs::Mipped folk[2];
    gs::Image bead, buffer, tick, shadow, sill, gauge, bird, cloud, sun;
    gs::Image title, levelWord, missWord;
    float lipY = 38.f;
    int font[6][96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

inline void trackBasis(float& nx, float& ny) {
    float dx = kX1 - kX0, dy = kY1 - kY0;
    float len = std::sqrt(dx * dx + dy * dy);
    nx = -dy / len;
    ny = dx / len;
}

// side: +1 red (near, earth side), -1 green (far), 0 centerline.
// extra moves along the earth normal; positive is down the bank.
inline void railPoint(float s, int side, float& x, float& y, float extra = 0.f) {
    float t = s / kLen;
    x = kX0 + (kX1 - kX0) * t;
    y = kY0 + (kY1 - kY0) * t;
    float nx, ny;
    trackBasis(nx, ny);
    float sc = s < 0.f ? 0.f : (s > kLen ? kLen : s);
    float bulge = 0.f;
    if (sc > 38.f && sc < 66.f) {
        float u = (sc - 38.f) / 28.f;
        bulge = std::sin(u * 3.14159265f);
    }
    float lat = extra;
    if (side > 0) lat += 12.f + bulge * 18.f;
    else if (side < 0) lat += -16.f - bulge * 18.f;
    x += nx * lat;
    y += ny * lat;
}

}  // namespace funicular
