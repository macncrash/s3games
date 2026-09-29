// S3 STRIKERTAPE pictures. Drawn at boot. No asset files.
// The tape wants BELL, GOLD and DING. TIN pays the same 10 as BELL and stays out.
#pragma once

#include "console/gfx.h"
#include "console/vdp.h"

namespace strikertape {

enum Pal {
    PAL_HUD = 0,
    PAL_WOOD = 1,
    PAL_BRASS = 2,
    PAL_PUCK = 3,
    PAL_MAN = 4,
    PAL_BELL = 5,
    PAL_GOLD = 6,
    PAL_DING = 7,
    PAL_TIN = 8,
    PAL_PAPER = 9,
    PAL_RED = 10,
    PAL_NIGHT = 11
};

constexpr int kTapeN = 3;
constexpr int kBandN = 4;
constexpr int kMaxSwings = 8;

struct Band {
    const char* name;
    int pay;
    int line;  // 0..2 on the tape, -1 if the twin stays out
    float p0, p1;
};

// High strike first. TIN sits between GOLD and BELL and is not the tape.
constexpr Band kBand[kBandN] = {
    {"BELL", 10, 0, 0.80f, 0.90f},
    {"GOLD", 7, 1, 0.46f, 0.56f},
    {"DING", 4, 2, 0.18f, 0.28f},
    {"TIN", 10, -1, 0.62f, 0.72f},
};

constexpr float kTowerX = 168.f;
constexpr float kTowerTop = 28.f;
constexpr float kTowerBot = 188.f;
constexpr float kManX = 78.f;
constexpr float kManY = 168.f;

inline int tapeSum() {
    int s = 0;
    for (int i = 0; i < kTapeN; i++) s += kBand[i].pay;
    return s;
}

inline float bandMid(int i) { return (kBand[i].p0 + kBand[i].p1) * 0.5f; }

struct Art {
    gs::Mipped tower;
    gs::Mipped puck;
    gs::Mipped bell;
    gs::Mipped mallet;
    gs::Mipped man;
    gs::Mipped slip;
    gs::Mipped slot;
    gs::Mipped lamp;
    int font[96] = {};
    int sky = 1;
    int dirt = 1;
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace strikertape
