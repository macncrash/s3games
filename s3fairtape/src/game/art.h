// S3 FAIRTAPE pictures and the shelf the rules use.
// Drawn at boot. Nothing is loaded from a file.
//
// The tape wants ROSE, CANE and BEAR.
// BUD, STICK and CUB pay those same counts and are not the tape.
#pragma once

#include <cmath>
#include <cstring>

#include "console/gfx.h"
#include "console/vdp.h"

namespace fairtape {

constexpr int kShelfN = 6;
constexpr int kTapeN = 3;
constexpr float kHit = 13.f;
constexpr float kShelf0 = 72.f;
constexpr float kShelfGap = 36.f;

enum class Shape : uint8_t { Rose, Bud, Cane, Stick, Bear, Cub };
enum class Fate : uint8_t { Firm, Short, Hot };

struct Shelf {
    const char* name;
    int pay;
    int line;  // 0..2 on the tape, -1 if the twin stays out
    Shape shape;
};

// Left to right. Twins sit beside the prizes they imitate.
constexpr Shelf kShelf[kShelfN] = {
    {"STICK", 24, -1, Shape::Stick},
    {"ROSE", 15, 0, Shape::Rose},
    {"CUB", 40, -1, Shape::Cub},
    {"CANE", 24, 1, Shape::Cane},
    {"BUD", 15, -1, Shape::Bud},
    {"BEAR", 40, 2, Shape::Bear},
};

enum Pal {
    PAL_INK = 0,
    PAL_GOLD = 1,
    PAL_GOOD = 2,
    PAL_BAD = 3,
    PAL_WOOD = 4,
    PAL_ROSE = 5,
    PAL_CANE = 6,
    PAL_BEAR = 7,
    PAL_RING = 8,
    PAL_KID = 9,
    PAL_BULB = 10,
    PAL_AWN = 11,
    PAL_NIGHT = 12,
    PAL_PAPER = 13,
    PAL_CREAM = 14,
    PAL_BLUE = 15
};

inline float shelfX(int i) { return kShelf0 + float(i) * kShelfGap; }

inline int shelfAt(float x) {
    int best = -1;
    float bestD = kHit + 1.f;
    for (int i = 0; i < kShelfN; i++) {
        float d = std::fabs(x - shelfX(i));
        if (d <= kHit && d < bestD) {
            bestD = d;
            best = i;
        }
    }
    return best;
}

inline const char* tapeName(int line) {
    for (int i = 0; i < kShelfN; i++)
        if (kShelf[i].line == line) return kShelf[i].name;
    return "";
}

inline int tapePay(int line) {
    for (int i = 0; i < kShelfN; i++)
        if (kShelf[i].line == line) return kShelf[i].pay;
    return 0;
}

inline int tapeSum() {
    int s = 0;
    for (int i = 0; i < kTapeN; i++) s += tapePay(i);
    return s;
}

inline int shelfOfLine(int line) {
    for (int i = 0; i < kShelfN; i++)
        if (kShelf[i].line == line) return i;
    return -1;
}

inline int twinShelf(int line) {
    int pay = tapePay(line);
    const char* name = tapeName(line);
    for (int i = 0; i < kShelfN; i++) {
        if (kShelf[i].pay != pay) continue;
        if (std::strcmp(kShelf[i].name, name) == 0) continue;
        return i;
    }
    return -1;
}

// A firm ring on a tape bottle is that line. Anything else stays out.
inline int takenLine(Fate fate, float x) {
    if (fate != Fate::Firm) return -1;
    int s = shelfAt(x);
    if (s < 0) return -1;
    return kShelf[s].line;
}

inline bool isTwin(Fate fate, float x) {
    if (fate != Fate::Firm) return false;
    int s = shelfAt(x);
    if (s < 0) return false;
    return kShelf[s].line < 0;
}

// Till slots follow the tape, left to right: ROSE, CANE, BEAR.
inline float slotX(int line) { return 100.f + float(line) * 72.f; }

struct Art {
    int font[96] = {};
    gs::Mipped jar, rose, bud, cane, stick, bear, cub;
    gs::Mipped ring, kid[2], pennant, bulb, post, shelf, awning, cloth;
    gs::Mipped wheel, moon, star, balloon, gondola, dot, shade;
    gs::Image logo, leave, stay, sign;
    gs::Image tag[kShelfN];
    gs::Image slip[kTapeN];
    gs::Image rail, tape, drawer;
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace fairtape
