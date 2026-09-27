// S3 SKATETAPE pictures. Drawn at boot. Nothing is loaded from a file.
// The tape wants OLLIE, GRIND and KICK.
// POP, SLIDE and SHOVE pay those same counts and are not the tape.
#pragma once

#include <cmath>
#include <cstring>

#include "console/gfx.h"
#include "console/vdp.h"

namespace skate {

constexpr int kMarkN = 6;
constexpr int kTapeN = 3;
constexpr int kMaxPops = 8;
constexpr float kHit = 22.f;
constexpr float kSweet = 0.07f;

enum class Trick : uint8_t { Ollie, Pop, Grind, Slide, Kick, Shove };

struct Mark {
    const char* name;
    int pay;
    int line;  // 0..2 on the tape, -1 if the twin stays out
    Trick trick;
    float x;
};

// Street positions, left to right. Twins sit beside the tape line they copy.
constexpr Mark kMark[kMarkN] = {
    {"SHOVE", 1, -1, Trick::Shove, 210.f},
    {"KICK", 1, 2, Trick::Kick, 310.f},
    {"POP", 2, -1, Trick::Pop, 470.f},
    {"OLLIE", 2, 0, Trick::Ollie, 590.f},
    {"SLIDE", 3, -1, Trick::Slide, 760.f},
    {"GRIND", 3, 1, Trick::Grind, 880.f},
};

enum Pal {
    PAL_INK = 0,
    PAL_GOLD = 1,
    PAL_GOOD = 2,
    PAL_BAD = 3,
    PAL_YOU = 4,
    PAL_DECK = 5,
    PAL_TAPE = 6,
    PAL_STREET = 7,
    PAL_PAPER = 8,
    PAL_WOOD = 9,
    PAL_WIN = 10
};

inline bool firmMeter(float m) { return std::fabs(m - 0.5f) <= kSweet; }

inline int markAt(float x) {
    int best = -1;
    float bestD = kHit + 1.f;
    for (int i = 0; i < kMarkN; i++) {
        float d = std::fabs(x - kMark[i].x);
        if (d <= kHit + 1e-3f && d < bestD) {
            bestD = d;
            best = i;
        }
    }
    return best;
}

inline const char* tapeName(int line) {
    for (int i = 0; i < kMarkN; i++)
        if (kMark[i].line == line) return kMark[i].name;
    return "";
}

inline int tapePay(int line) {
    for (int i = 0; i < kMarkN; i++)
        if (kMark[i].line == line) return kMark[i].pay;
    return 0;
}

inline int tapeSum() {
    int s = 0;
    for (int i = 0; i < kTapeN; i++) s += tapePay(i);
    return s;
}

inline int markOfLine(int line) {
    for (int i = 0; i < kMarkN; i++)
        if (kMark[i].line == line) return i;
    return -1;
}

inline int takenLine(bool firm, float x) {
    if (!firm) return -1;
    int m = markAt(x);
    if (m < 0) return -1;
    return kMark[m].line;
}

struct Art {
    int font[96] = {};
    gs::Mipped skater;
    gs::Mipped board;
    gs::Image post;
    gs::Image cassette;
    gs::Image drawer;
    gs::Image slot;
    gs::Image slip[kTapeN];
    gs::Image wheel;
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace skate
