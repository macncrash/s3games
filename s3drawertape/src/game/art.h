// S3 DRAWERTAPE pictures. Drawn at boot. No asset files.
#pragma once

#include "console/gfx.h"
#include "console/vdp.h"

namespace drawertape {

enum Pal {
    PAL_TEXT = 0,
    PAL_AMBER = 1,
    PAL_RED = 2,
    PAL_GREEN = 3,
    PAL_DESK = 4,
    PAL_PAPER = 5,
    PAL_BRASS = 6,
    PAL_LAMP = 7,
    PAL_INK = 8,
    PAL_WOOD = 9,
    PAL_DECOY = 10
};

constexpr int kBins = 4;
constexpr int kTapeN = 3;

// The tape is the first three. TAG pays the same as STAMP and stays out.
struct Piece {
    const char* name;
    int pay;
    bool decoy;
};

constexpr Piece kPiece[kBins] = {
    {"CLIP", 4, false},
    {"STAMP", 7, false},
    {"KEY", 3, false},
    {"TAG", 7, true},
};

constexpr int kTape[kTapeN] = {0, 1, 2};

struct Art {
    gs::Mipped tray;
    gs::Mipped lamp;
    gs::Mipped hand;
    gs::Mipped slip;
    gs::Mipped solid;
    gs::Mipped mark[kBins];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace drawertape
