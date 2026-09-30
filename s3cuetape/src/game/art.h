// S3 CUETAPE pictures. Drawn at boot. No asset files.
#pragma once

#include "console/gfx.h"
#include "console/vdp.h"

namespace cuetape {

enum Pal {
    PAL_HUD = 0,
    PAL_PAPER = 1,
    PAL_CLOTH = 2,
    PAL_WOOD = 3,
    PAL_CUE = 4,
    PAL_BALL = 5,
    PAL_GOLD = 6,
    PAL_PLAYER = 7,
    PAL_CHALK = 8,
    PAL_BAD = 9,
    PAL_SLOT = 10,
    PAL_SHADE = 11
};

constexpr int kTapeN = 3;
constexpr int kMaxStrokes = 6;
constexpr float kHandX = 160.f;
constexpr float kHandY = 188.f;
constexpr float kDist0 = 70.f;
constexpr float kDist1 = 160.f;

struct Pocket {
    const char* name;
    int pay;
    float x, y, r;
    bool decoy;
};

// The tape is the first three, in order. FOUL pays the same as BREAK and stays out.
constexpr Pocket kPocket[4] = {
    {"BREAK", 6, 160.f, 58.f, 16.f, false},
    {"OBJECT", 5, 78.f, 96.f, 16.f, false},
    {"CUE", 3, 230.f, 112.f, 15.f, false},
    {"FOUL", 6, 210.f, 58.f, 14.f, true},
};

struct Art {
    gs::Mipped table;
    gs::Mipped cue;
    gs::Mipped ball;
    gs::Mipped object;
    gs::Mipped chalk;
    gs::Mipped pocket;
    gs::Mipped slip;
    gs::Mipped slot;
    gs::Mipped drawer;
    gs::Mipped player;
    int font[96] = {};
    int felt = 1;
    int rail = 1;
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace cuetape
