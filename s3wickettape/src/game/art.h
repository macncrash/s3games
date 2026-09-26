// S3 WICKETTAPE pictures and the six strokes the rules use.
// Drawn at boot. Nothing is loaded from a file.
//
// The tape wants GLANCE, DRIVE and SIX.
// NUDGE, PUSH and LOFT pay those same counts and are not the tape.
#pragma once

#include <cmath>
#include <cstring>

#include "console/gfx.h"
#include "console/vdp.h"

namespace wickettape {

constexpr int kGapN = 6;
constexpr int kTapeN = 3;
constexpr int kMaxBalls = 6;
constexpr float kSweet = 0.08f;

constexpr float kSlotX[kTapeN] = {112.f, 180.f, 248.f};
constexpr float kSlotY = 208.f;

enum class Stroke : uint8_t { Glance = 0, Nudge = 1, Drive = 2, Push = 3, Six = 4, Loft = 5 };

// tx,ty is where the ball finishes. fx,fy is where that fielder stands.
struct Gap {
    const char* name;
    int pay;
    int line;  // 0..2 on the tape, -1 if the twin stays out
    Stroke stroke;
    float tx, ty;
    float fx, fy;
    float apex;
};

constexpr Gap kGap[kGapN] = {
    {"NUDGE", 1, -1, Stroke::Nudge, 116.f, 146.f, 116.f, 154.f, 8.f},
    {"GLANCE", 1, 0, Stroke::Glance, 40.f, 128.f, 46.f, 118.f, 6.f},
    {"PUSH", 3, -1, Stroke::Push, 150.f, 104.f, 150.f, 128.f, 16.f},
    {"DRIVE", 3, 1, Stroke::Drive, 248.f, 120.f, 248.f, 138.f, 12.f},
    {"LOFT", 6, -1, Stroke::Loft, 186.f, 52.f, 186.f, 106.f, 40.f},
    {"SIX", 6, 2, Stroke::Six, 308.f, 40.f, 270.f, 100.f, 52.f},
};

enum Pal {
    PAL_INK = 0,
    PAL_GOLD = 1,
    PAL_GOOD = 2,
    PAL_BAD = 3,
    PAL_SKY = 4,
    PAL_KIT = 5,
    PAL_BOWL = 6,
    PAL_BALL = 7,
    PAL_WOOD = 8,
    PAL_GRASS = 9,
    PAL_FIELD = 10,
    PAL_TWIN = 11,
    PAL_PAPER = 12,
    PAL_ROPE = 13,
    PAL_CROWD = 14,
    PAL_HOUSE = 15
};

inline bool sweetMeter(float m) { return std::fabs(m - 0.5f) <= kSweet; }

inline const char* tapeName(int line) {
    for (int i = 0; i < kGapN; i++)
        if (kGap[i].line == line) return kGap[i].name;
    return "";
}

inline int tapePay(int line) {
    for (int i = 0; i < kGapN; i++)
        if (kGap[i].line == line) return kGap[i].pay;
    return 0;
}

inline int tapeSum() {
    int s = 0;
    for (int i = 0; i < kTapeN; i++) s += tapePay(i);
    return s;
}

inline int gapOfLine(int line) {
    for (int i = 0; i < kGapN; i++)
        if (kGap[i].line == line) return i;
    return -1;
}

inline int twinOf(int line) {
    int pay = tapePay(line);
    const char* name = tapeName(line);
    for (int i = 0; i < kGapN; i++) {
        if (kGap[i].pay != pay) continue;
        if (std::strcmp(kGap[i].name, name) == 0) continue;
        return i;
    }
    return -1;
}

// A firm stroke on a tape gap is that line. A twin, or a mistimed stroke, stays out.
inline int takenLine(bool firm, int gap) {
    if (!firm || gap < 0 || gap >= kGapN) return -1;
    return kGap[gap].line;
}

inline bool isTwin(bool firm, int gap) {
    if (!firm || gap < 0 || gap >= kGapN) return false;
    if (kGap[gap].line >= 0) return false;
    for (int t = 0; t < kTapeN; t++)
        if (kGap[gap].pay == tapePay(t)) return true;
    return false;
}

struct Art {
    int font[96] = {};
    gs::Image batsman[5];
    gs::Image bowler[3];
    gs::Image keeper;
    gs::Image fielder;
    gs::Image ball[2];
    gs::Image stump;
    gs::Image pitch;
    gs::Image rope;
    gs::Image tree;
    gs::Image crowd;
    gs::Image house;
    gs::Image screen;
    gs::Image cloud;
    gs::Image sun;
    gs::Image shadow;
    gs::Image blot;
    gs::Image tape;
    gs::Image drawer;
    gs::Image slot;
    gs::Image logo;
    gs::Image matchW;
    gs::Image openW;
    gs::Image slip[kTapeN];
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace wickettape
