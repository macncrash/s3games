// S3 BOARDTAPE pictures. Drawn at boot. No asset files.
#pragma once

#include "console/gfx.h"
#include "console/vdp.h"

namespace boardtape {

enum Pal {
    PAL_TEXT = 0,
    PAL_AMBER = 1,
    PAL_RED = 2,
    PAL_GREEN = 3,
    PAL_BOARD = 4,
    PAL_PAPER = 5,
    PAL_BRASS = 6,
    PAL_LAMP = 7,
    PAL_INK = 8,
    PAL_CORD = 9,
    PAL_NIGHT = 10
};

constexpr int kJacks = 4;
constexpr int kTapeN = 3;

// The tape is the first three. NIGHT pays the same as TRUNK and stays out.
struct Call {
    const char* name;
    int pay;
    bool decoy;
};

constexpr Call kCall[kJacks] = {
    {"LOCAL", 4, false},
    {"TRUNK", 7, false},
    {"TOLL", 3, false},
    {"NIGHT", 7, true},
};

constexpr int kTape[kTapeN] = {0, 1, 2};

struct Art {
    gs::Mipped jack;
    gs::Mipped lamp;
    gs::Mipped plug;
    gs::Mipped slip;
    gs::Mipped solid;
    gs::Mipped stamp[kJacks];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace boardtape
