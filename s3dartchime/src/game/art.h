// S3 DARTCHIME pictures. Drawn at boot. No asset files.
// The radii are the scoring rules as well as the paint.
#pragma once

#include <cmath>
#include <cstdio>

#include "console/gfx.h"
#include "console/vdp.h"

namespace dartchime {

constexpr int kBmp = 150;
constexpr float kBmpC = 75.f;
constexpr int kBoardX = 34;
constexpr int kBoardY = 42;
constexpr float kCx = kBoardX + kBmpC;
constexpr float kCy = kBoardY + kBmpC;

// Beds, bitmap pixels from the board centre. Shared with the rules.
constexpr float kBullIn = 5.5f;
constexpr float kBullOut = 12.f;
constexpr float kTripIn = 33.f;
constexpr float kTripOut = 42.f;
constexpr float kDoubIn = 54.f;
constexpr float kDoubOut = 63.f;
constexpr float kNumR = 68.f;
constexpr float kRim = 73.f;

// Clockwise from the top, match order. 20 is the black bed at twelve o'clock.
constexpr int kSeg[20] = {20, 1, 18, 4, 13, 6, 10, 15, 2, 17, 3, 19, 7, 16, 8, 11, 14, 9, 12, 5};
constexpr int kHourSeg = 18;  // the number 12
constexpr float kHourRad = (kDoubIn + kDoubOut) * 0.5f;

static_assert(kSeg[kHourSeg] == 12, "hour bed is double twelve");
static_assert(kHourRad > kDoubIn && kHourRad < kDoubOut, "hour mark sits in the double");

constexpr float kClockX = 258.f;
constexpr float kClockY = 118.f;
constexpr float kClockS = 66.f;
constexpr float kMeterY = 34.f;

enum Pal {
    PAL_BOARD = 0,
    PAL_INK = 1,
    PAL_GOLD = 2,
    PAL_ALERT = 3,
    PAL_GREEN = 4,
    PAL_AIM = 5,
    PAL_DART = 6,
    PAL_CLOCK = 7,
    PAL_HAND = 8,
    PAL_BELL = 9,
    PAL_TITLE = 10,
    PAL_WOOD = 11,
    PAL_FLAME = 12,
    PAL_CHALK = 13
};

enum class Bed : uint8_t { Miss, Bull, Outer, Single, Triple, Double };

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
    if (r <= kBullIn) {
        m.bed = Bed::Bull;
        m.number = 50;
        m.mul = 1;
        return m;
    }
    if (r <= kBullOut) {
        m.bed = Bed::Outer;
        m.number = 25;
        m.mul = 1;
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

inline bool hourBed(const Mark& m) { return m.bed == Bed::Double && m.number == 12; }

inline void markName(const Mark& m, char* out, int n) {
    if (!out || n <= 0) return;
    if (m.bed == Bed::Bull) std::snprintf(out, size_t(n), "BULL");
    else if (m.bed == Bed::Outer) std::snprintf(out, size_t(n), "25");
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
    gs::Image face;
    gs::Image ring;
    gs::Image cap;
    gs::Image hand[3][60];
    gs::Image bell;
    gs::Image yoke;
    gs::Mipped dart;
    gs::Image cross;
    gs::Image pip;
    gs::Image shadow;
    gs::Image dot;
    gs::Image flame;
    gs::Image oche;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace dartchime
