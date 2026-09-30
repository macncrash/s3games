// S3 HORNTAPE pictures. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace horntape {

constexpr int kParts = 4;
constexpr int kTapeN = 3;
constexpr int kDieAt = 64;
constexpr int kSweetLo = 22;
constexpr int kSweetHi = 36;

enum Pal {
    PAL_HUD = 0,
    PAL_GOLD = 1,
    PAL_DIM = 2,
    PAL_BAD = 3,
    PAL_LIP = 4,
    PAL_VALVE = 5,
    PAL_FLARE = 6,
    PAL_SQUEAL = 7,
    PAL_BRASS = 8,
    PAL_PAPER = 9,
    PAL_AIR = 10
};

// The tape is LIP, VALVE, FLARE. SQUEAL pays like VALVE and stays out.
struct Part {
    const char* name;
    int pay;
    bool decoy;
    int pal;
};

constexpr Part kPart[kParts] = {
    {"LIP", 5, false, PAL_LIP},
    {"VALVE", 8, false, PAL_VALVE},
    {"FLARE", 6, false, PAL_FLARE},
    {"SQUEAL", 8, true, PAL_SQUEAL},
};

constexpr int kTape[kTapeN] = {0, 1, 2};

struct Art {
    gs::Image horn;
    gs::Image bell;
    gs::Image lip;
    gs::Image valve;
    gs::Image flare;
    gs::Image squeal;
    gs::Image drawer;
    gs::Image breath;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace horntape
