// S3 DARTBELL pictures. Drawn at boot. No asset files.
// The radii are the scoring rules as well as the paint.
#pragma once

#include <cmath>
#include <cstdio>

#include "console/gfx.h"
#include "console/vdp.h"

namespace dartbell {

constexpr int kBmp = 144;
constexpr float kBmpC = 72.f;
constexpr int kBoardX = 88;
constexpr int kBoardY = 42;
constexpr float kCx = 160.f;
constexpr float kCy = 114.f;

// The bell replaces the bull. Brass around it is the mount and does not ring.
constexpr float kBellR = 11.f;
constexpr float kLipOut = 18.f;
constexpr float kTripIn = 30.f;
constexpr float kTripOut = 38.f;
constexpr float kDoubIn = 49.f;
constexpr float kDoubOut = 57.f;
constexpr float kNumR = 63.f;
constexpr float kRim = 70.f;

constexpr int kSeg[20] = {20, 1, 18, 4, 13, 6, 10, 15, 2, 17, 3, 19, 7, 16, 8, 11, 14, 9, 12, 5};

enum Pal {
    PAL_BOARD = 0,
    PAL_INK = 1,
    PAL_GOLD = 2,
    PAL_GREEN = 3,
    PAL_ALERT = 4,
    PAL_AIM = 5,
    PAL_BELL = 6,
    PAL_DART = 7,
    PAL_TITLE = 8,
    PAL_WOOD = 9,
    PAL_FLAME = 10,
    PAL_WAX = 11,
    PAL_CHALK = 12
};

enum class Bed : uint8_t { Miss, Bell, Mount, Single, Triple, Double };

struct Mark {
    Bed bed = Bed::Miss;
    int number = 0;
    int mul = 0;
};

inline int sectorAt(float dx, float dy) {
    float deg = std::atan2(dx, -dy) * (180.f / 3.14159265f);
    if (deg < 0.f) deg += 360.f;
    int s = int(std::floor((deg + 9.f) / 18.f));
    if (s < 0 || s >= 20) s = 0;
    return s;
}

inline void bedPoint(int seg, float rad, float& x, float& y) {
    float ang = float(seg) * (3.14159265f / 10.f);
    x = kCx + std::sin(ang) * rad;
    y = kCy - std::cos(ang) * rad;
}

inline Mark classify(float x, float y) {
    Mark m;
    float dx = x - kCx;
    float dy = y - kCy;
    float r = std::hypot(dx, dy);
    if (r > kDoubOut) return m;
    if (r <= kBellR) {
        m.bed = Bed::Bell;
        return m;
    }
    if (r <= kLipOut) {
        m.bed = Bed::Mount;
        return m;
    }
    m.number = kSeg[sectorAt(dx, dy)];
    if (r <= kTripIn) {
        m.bed = Bed::Single;
        m.mul = 1;
    } else if (r <= kTripOut) {
        m.bed = Bed::Triple;
        m.mul = 3;
    } else if (r <= kDoubIn) {
        m.bed = Bed::Single;
        m.mul = 1;
    } else {
        m.bed = Bed::Double;
        m.mul = 2;
    }
    return m;
}

inline void markName(const Mark& m, char* out, int n) {
    if (!out || n <= 0) return;
    if (m.bed == Bed::Bell) std::snprintf(out, size_t(n), "BELL");
    else if (m.bed == Bed::Mount) std::snprintf(out, size_t(n), "BRASS");
    else if (m.bed == Bed::Miss) std::snprintf(out, size_t(n), "MISS");
    else {
        char k = 'S';
        if (m.bed == Bed::Triple) k = 'T';
        else if (m.bed == Bed::Double) k = 'D';
        std::snprintf(out, size_t(n), "%c%d", k, m.number);
    }
}

struct Art {
    gs::Image board;
    gs::Image title;
    gs::Image rung;
    gs::Image dead;
    gs::Mipped bell;
    gs::Image clapper;
    gs::Image yoke;
    gs::Mipped dart;
    gs::Image cross;
    gs::Image pip;
    gs::Image shadow;
    gs::Image dot;
    gs::Image beam;
    gs::Image candle;
    gs::Image flame;
    gs::Image oche;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace dartbell
