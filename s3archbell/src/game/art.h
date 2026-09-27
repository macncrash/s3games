// S3 ARCHBELL pictures. Drawn at boot. No asset files.
// The radii are the scoring rules as well as the paint.
#pragma once

#include <cmath>

#include "console/vdp.h"

namespace archbell {

constexpr int kFace = 148;
constexpr float kFaceMid = 74.f;
constexpr float kCx = 232.f;
constexpr float kCy = 108.f;

// The bell hangs in the gold. Gold around it does not ring.
constexpr float kBellR = 10.f;
constexpr float kGold = 22.f;
constexpr float kRed = 36.f;
constexpr float kBlue = 50.f;
constexpr float kBlack = 62.f;
constexpr float kWhite = 72.f;
constexpr float kBoss = 78.f;

constexpr float kArchX = 2.f;
constexpr float kArchY = 72.f;
constexpr float kLooseX = 86.f;
constexpr float kLooseY = 118.f;

enum Pal {
    PAL_FACE = 0,
    PAL_ARCH = 1,
    PAL_ARROW = 2,
    PAL_SIGHT = 3,
    PAL_BELL = 4,
    PAL_WORLD = 5,
    PAL_INK = 6,
    PAL_GOLD = 7,
    PAL_ALERT = 8,
    PAL_GREEN = 9,
    PAL_WORD = 10
};

enum class Bed : uint8_t { Miss, Bell, Gold, Red, Blue, Black, White, Straw };

struct Mark {
    Bed bed = Bed::Miss;
    const char* name = "MISS";
};

inline Mark classify(float x, float y) {
    float r = std::hypot(x - kCx, y - kCy);
    if (r <= kBellR) return {Bed::Bell, "BELL"};
    if (r <= kGold) return {Bed::Gold, "GOLD"};
    if (r <= kRed) return {Bed::Red, "RED"};
    if (r <= kBlue) return {Bed::Blue, "BLUE"};
    if (r <= kBlack) return {Bed::Black, "BLACK"};
    if (r <= kWhite) return {Bed::White, "WHITE"};
    if (r <= kBoss) return {Bed::Straw, "STRAW"};
    return {Bed::Miss, "MISS"};
}

struct Art {
    gs::Image face;
    gs::Image bell;
    gs::Image archer;
    gs::Image arrow;
    gs::Image sight;
    gs::Image stand;
    gs::Image quiver;
    gs::Image shaft;
    gs::Image word;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace archbell
