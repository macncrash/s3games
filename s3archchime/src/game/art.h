// S3 ARCHCHIME pictures. Drawn at boot. No asset files.
// The radii are the scoring rules as well as the paint.
#pragma once

#include <cmath>

#include "console/vdp.h"

namespace archchime {

constexpr int kFace = 128;
constexpr float kFaceMid = 64.f;
constexpr float kCx = 228.f;
constexpr float kCy = 116.f;

// Inner gold is the only bed the hour will answer.
constexpr float kGold = 13.f;
constexpr float kRed = 26.f;
constexpr float kBlue = 38.f;
constexpr float kBlack = 50.f;
constexpr float kWhite = 58.f;
constexpr float kStraw = 63.f;

constexpr float kArchX = 6.f;
constexpr float kArchY = 78.f;
constexpr float kLooseX = 98.f;
constexpr float kLooseY = kCy;

constexpr int kArrows = 3;
constexpr int kGraceSec = 24;
constexpr int kFpc = 6;
constexpr int kHourSec = 12 * 3600;

constexpr float kWindPx = 12.f;
constexpr int kWind0 = 3;

enum Pal {
    PAL_FACE = 0,
    PAL_ARCH = 1,
    PAL_ARROW = 2,
    PAL_SIGHT = 3,
    PAL_CLOCK = 4,
    PAL_WORLD = 5,
    PAL_INK = 6,
    PAL_GOLD = 7,
    PAL_ALERT = 8,
    PAL_GREEN = 9,
    PAL_WORD = 10
};

enum class Bed : uint8_t { Miss, Gold, Red, Blue, Black, White, Straw };

struct Mark {
    Bed bed = Bed::Miss;
    const char* name = "MISS";
};

inline Mark classify(float x, float y) {
    float r = std::hypot(x - kCx, y - kCy);
    if (r <= kGold) return {Bed::Gold, "GOLD"};
    if (r <= kRed) return {Bed::Red, "RED"};
    if (r <= kBlue) return {Bed::Blue, "BLUE"};
    if (r <= kBlack) return {Bed::Black, "BLACK"};
    if (r <= kWhite) return {Bed::White, "WHITE"};
    if (r <= kStraw) return {Bed::Straw, "STRAW"};
    return {Bed::Miss, "MISS"};
}

struct Art {
    gs::Image face;
    gs::Image archer;
    gs::Image arrow;
    gs::Image sight;
    gs::Image clock;
    gs::Image stand;
    gs::Image tree;
    gs::Image belfry;
    gs::Image pip;
    gs::Image word;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace archchime
