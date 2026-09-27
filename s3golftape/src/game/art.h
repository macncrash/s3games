// S3 GOLFTAPE pictures and the card the rules use.
// Drawn at boot. Nothing is loaded from a file.
//
// The tape wants FADE, then PITCH, then DROP.
// DRAW, FLIP and LIP pay those same counts and are not the tape.
#pragma once

#include "console/gfx.h"
#include "console/vdp.h"

namespace golftape {

constexpr int kTapeN = 3;
constexpr float kFirmLo = 0.42f;
constexpr float kFirmHi = 0.58f;
constexpr float kCupX = 262.f;
constexpr float kGround = 168.f;

inline bool firmMeter(float m) { return m >= kFirmLo && m <= kFirmHi; }

inline bool inFade(float a) { return a >= 0.22f && a <= 0.62f; }
inline bool inDraw(float a) { return a <= -0.22f && a >= -0.62f; }
inline bool inPitch(float a) { return a >= -0.18f && a <= 0.18f; }
inline bool inFlip(float a) { return a >= 0.38f && a <= 0.72f; }
inline bool inDrop(float a) { return a >= -0.14f && a <= 0.14f; }
inline bool inLip(float a) { return a >= 0.24f && a <= 0.48f; }

inline const char* tapeName(int i) {
    if (i == 0) return "FADE";
    if (i == 1) return "PITCH";
    if (i == 2) return "DROP";
    return "";
}

inline int tapePay(int i) {
    if (i == 0) return 4;
    if (i == 1) return 3;
    if (i == 2) return 1;
    return 0;
}

inline const char* twinName(int i) {
    if (i == 0) return "DRAW";
    if (i == 1) return "FLIP";
    if (i == 2) return "LIP";
    return "";
}

inline int tapeSum() {
    int s = 0;
    for (int i = 0; i < kTapeN; i++) s += tapePay(i);
    return s;
}

enum Pal {
    PAL_INK = 0,
    PAL_GOLD = 1,
    PAL_GREEN = 2,
    PAL_ALERT = 3,
    PAL_PAPER = 4,
    PAL_WORLD = 5,
    PAL_BALL = 6,
    PAL_MAN = 7,
    PAL_FLAG = 8,
    PAL_SLIP = 9,
    PAL_TITLE = 10,
    PAL_WOOD = 11
};

struct Art {
    gs::Image course;
    gs::Image paper;
    gs::Image drawer;
    gs::Image title;
    gs::Mipped ball;
    gs::Mipped flag[2];
    gs::Mipped golfer[2];
    gs::Image slip[3];
    gs::Image dot;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace golftape
