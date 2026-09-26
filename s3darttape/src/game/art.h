// S3 DARTTAPE pictures and the board the rules use.
// Drawn at boot. Nothing is loaded from a file.
//
// The tape wants the three gold doubles: D15, D12, D18.
// T10, T8 and T12 score those same counts and are not the tape.
#pragma once

#include <cmath>
#include <cstdio>

#include "console/gfx.h"
#include "console/vdp.h"

namespace darttape {

constexpr int kBmp = 144;
constexpr float kBmpC = 72.f;
constexpr int kBoardX = 104;
constexpr int kBoardY = 36;
constexpr float kCx = 176.f;
constexpr float kCy = 108.f;

constexpr float kBullIn = 7.f;
constexpr float kBullOut = 16.f;
constexpr float kTripIn = 32.f;
constexpr float kTripOut = 40.f;
constexpr float kDoubIn = 52.f;
constexpr float kDoubOut = 62.f;
constexpr float kRadInner = 24.f;
constexpr float kRadTriple = 36.f;
constexpr float kRadSingle = 46.f;
constexpr float kRadDouble = 57.f;
constexpr float kNumR = 66.6f;
constexpr float kRim = 71.f;

constexpr float kDrawerL = 108.f;
constexpr float kDrawerT = 182.f;
constexpr int kDrawerW = 156;
constexpr int kDrawerH = 24;
constexpr float kSlotY = 194.f;
constexpr float kPaperX = 6.f;
constexpr float kPaperY = 40.f;
constexpr int kPaperW = 90;
constexpr int kPaperH = 136;
constexpr float kPlayerX = 304.f;
constexpr float kPlayerY = 150.f;
constexpr float kHandX = 292.f;
constexpr float kHandY = 138.f;

inline float slotX(int i) { return 136.f + float(i) * 46.f; }

// Clockwise from the top, the standard twenty.
constexpr int kSeg[20] = {20, 1, 18, 4, 13, 6, 10, 15, 2, 17, 3, 19, 7, 16, 8, 11, 14, 9, 12, 5};

namespace rule {
inline int number(int i) {
    if (i == 1) return 12;
    if (i == 2) return 18;
    return 15;
}
inline int score(int i) { return number(i) * 2; }
inline int twin(int i) {
    if (i == 1) return 8;
    if (i == 2) return 12;
    return 10;
}
inline int twinScore(int i) { return twin(i) * 3; }
}  // namespace rule

enum Pal {
    PAL_BOARD = 0,
    PAL_INK = 1,
    PAL_GOLD = 2,
    PAL_GREEN = 3,
    PAL_ALERT = 4,
    PAL_AIM = 5,
    PAL_DART = 6,
    PAL_PAPER = 7,
    PAL_WOOD = 8,
    PAL_PLAYER = 9,
    PAL_TITLE = 10
};

enum class Kind : uint8_t { Miss, Single, Double, Triple, Bull, Outer };

struct Hit {
    Kind kind = Kind::Miss;
    int number = 0;
    int mul = 0;
    int score = 0;
};

inline int sectorOf(int number) {
    for (int s = 0; s < 20; s++)
        if (kSeg[s] == number) return s;
    return -1;
}

inline int sectorAt(float dx, float dy) {
    float deg = std::atan2(dx, -dy) * (180.f / 3.14159265f);
    if (deg < 0.f) deg += 360.f;
    int s = int(std::floor((deg + 9.f) / 18.f));
    if (s < 0 || s >= 20) s = 0;
    return s;
}

inline void bedPoint(int number, float rad, float& x, float& y) {
    int seg = sectorOf(number);
    if (seg < 0) {
        x = kCx;
        y = kCy;
        return;
    }
    float ang = float(seg) * (3.14159265f / 10.f);
    x = kCx + std::sin(ang) * rad;
    y = kCy - std::cos(ang) * rad;
}

inline Hit classify(float x, float y) {
    Hit h;
    float dx = x - kCx;
    float dy = y - kCy;
    float r = std::hypot(dx, dy);
    if (r > kDoubOut) return h;
    if (r <= kBullIn) {
        h.kind = Kind::Bull;
        h.number = 25;
        h.mul = 2;
        h.score = 50;
        return h;
    }
    if (r <= kBullOut) {
        h.kind = Kind::Outer;
        h.number = 25;
        h.mul = 1;
        h.score = 25;
        return h;
    }
    h.number = kSeg[sectorAt(dx, dy)];
    if (r <= kTripIn) {
        h.kind = Kind::Single;
        h.mul = 1;
    } else if (r <= kTripOut) {
        h.kind = Kind::Triple;
        h.mul = 3;
    } else if (r <= kDoubIn) {
        h.kind = Kind::Single;
        h.mul = 1;
    } else {
        h.kind = Kind::Double;
        h.mul = 2;
    }
    h.score = h.number * h.mul;
    return h;
}

inline void hitName(const Hit& h, char* out, int n) {
    if (!out || n <= 0) return;
    if (h.kind == Kind::Miss) std::snprintf(out, size_t(n), "MISS");
    else if (h.kind == Kind::Bull) std::snprintf(out, size_t(n), "BULL");
    else if (h.kind == Kind::Outer) std::snprintf(out, size_t(n), "OUTER");
    else {
        char k = 'S';
        if (h.kind == Kind::Triple) k = 'T';
        else if (h.kind == Kind::Double) k = 'D';
        std::snprintf(out, size_t(n), "%c%d", k, h.number);
    }
}

inline int tapeIndex(const Hit& h) {
    if (h.kind != Kind::Double) return -1;
    for (int i = 0; i < 3; i++)
        if (h.number == rule::number(i)) return i;
    return -1;
}

inline int countIndex(const Hit& h) {
    if (h.score <= 0) return -1;
    for (int i = 0; i < 3; i++)
        if (h.score == rule::score(i)) return i;
    return -1;
}

inline bool tapeNumberHit(const Hit& h) {
    if (h.kind == Kind::Miss || h.kind == Kind::Bull || h.kind == Kind::Outer) return false;
    for (int i = 0; i < 3; i++)
        if (h.number == rule::number(i)) return true;
    return false;
}

struct Art {
    gs::Image board;
    gs::Image title;
    gs::Image paid;
    gs::Image open;
    gs::Mipped dart;
    gs::Image cross;
    gs::Image pip;
    gs::Image shadow;
    gs::Image dot;
    gs::Image paper;
    gs::Image drawer;
    gs::Image slip[3];
    gs::Image player[2];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace darttape
