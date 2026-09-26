// S3 HOOPTAPE pictures and the six marks the rules use.
// Drawn at boot. Nothing is loaded from a file.
//
// The tape wants SWISH, BANK and FREE.
// IRON, ARC and RIM pay those same counts and are not the tape.
#pragma once

#include <cmath>
#include <cstring>

#include "console/gfx.h"
#include "console/vdp.h"

namespace hooptape {

constexpr int kMarkN = 6;
constexpr int kTapeN = 3;
constexpr int kMaxShots = 6;
constexpr float kHit = 0.48f;
constexpr float kSweet = 0.08f;
constexpr float kHomeX = -4.85f;
constexpr float kHomeZ = 0.55f;
constexpr float kMinX = -8.20f;
constexpr float kMaxX = -3.40f;
constexpr float kMinZ = -0.75f;
constexpr float kMaxZ = 1.95f;

// Drawer slots sit under the printed tape, left to right: SWISH, BANK, FREE.
constexpr float kSlotX[kTapeN] = {124.f, 188.f, 252.f};
constexpr float kSlotY = 204.f;

enum class Make : uint8_t { Swish, Iron, Bank, Arc, Free, Rim };

struct Mark {
    const char* name;
    int pay;
    int line;  // 0..2 on the tape, -1 if the twin stays out
    Make make;
    float x, z;
};

// Depth is world x (0 is the rim, negative is away from it). z is the line.
constexpr Mark kMark[kMarkN] = {
    {"RIM", 1, -1, Make::Rim, -4.00f, 1.20f},
    {"FREE", 1, 2, Make::Free, -4.00f, 0.00f},
    {"IRON", 2, -1, Make::Iron, -5.70f, 1.25f},
    {"SWISH", 2, 0, Make::Swish, -5.70f, 0.00f},
    {"ARC", 3, -1, Make::Arc, -7.60f, -0.20f},
    {"BANK", 3, 1, Make::Bank, -6.90f, 1.35f},
};

enum Pal {
    PAL_INK = 0,
    PAL_GOLD = 1,
    PAL_GOOD = 2,
    PAL_BAD = 3,
    PAL_YOU = 4,
    PAL_BALL = 5,
    PAL_IRON = 6,
    PAL_BOARD = 7,
    PAL_NET = 8,
    PAL_PAPER = 9,
    PAL_WOOD = 10,
    PAL_MARKW = 11,
    PAL_MARKB = 12,
    PAL_MARKG = 13,
    PAL_MARKR = 14,
    PAL_WIN = 15
};

inline bool firmMeter(float m) { return std::fabs(m - 0.5f) <= kSweet; }

inline int markAt(float x, float z) {
    int best = -1;
    float bestD = kHit + 1.f;
    for (int i = 0; i < kMarkN; i++) {
        float d = std::hypot(x - kMark[i].x, z - kMark[i].z);
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

inline int twinMark(int line) {
    int pay = tapePay(line);
    const char* name = tapeName(line);
    for (int i = 0; i < kMarkN; i++) {
        if (kMark[i].pay != pay) continue;
        if (std::strcmp(kMark[i].name, name) == 0) continue;
        return i;
    }
    return -1;
}

// A firm release on a tape mark is that line. Anything else stays out.
inline int takenLine(bool firm, float x, float z) {
    if (!firm) return -1;
    int m = markAt(x, z);
    if (m < 0) return -1;
    return kMark[m].line;
}

inline bool isTwin(bool firm, float x, float z) {
    if (!firm) return false;
    int m = markAt(x, z);
    if (m < 0) return false;
    return kMark[m].line < 0;
}

struct Art {
    int font[96] = {};
    gs::Mipped player;
    gs::Mipped ball[2];
    gs::Image rim;
    gs::Image net[2];
    gs::Image board;
    gs::Image pole;
    gs::Image arm;
    gs::Image base;
    gs::Image pip;
    gs::Image box;
    gs::Image blot;
    gs::Image shadow;
    gs::Image window;
    gs::Image key;
    gs::Image tape;
    gs::Image drawer;
    gs::Image slot;
    gs::Image logo;
    gs::Image matchW;
    gs::Image openW;
    gs::Image slip[kTapeN];
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace hooptape
