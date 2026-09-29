// S3 SOLITAIRETAPE pictures. Drawn at boot. No asset files.
#pragma once

#include "console/gfx.h"
#include "console/vdp.h"

namespace solitairetape {

enum Pal {
    PAL_FELT = 0,
    PAL_CARD = 1,
    PAL_PIP = 2,
    PAL_REEL = 3,
    PAL_INK = 4,
    PAL_TITLE = 5,
    PAL_WIN = 6,
    PAL_BAD = 7,
    PAL_HINT = 8,
    PAL_SLOT = 9
};

constexpr int kCards = 6;
constexpr int kTapeN = 3;

// ACE, FIVE, NINE are the tape. JACK pays the same 5 as FIVE and stays out.
struct Face {
    const char* name;
    int pay;
    bool decoy;
};

constexpr Face kFace[kCards] = {
    {"ACE", 1, false},
    {"JACK", 5, true},
    {"FIVE", 5, false},
    {"TWO", 2, false},
    {"NINE", 9, false},
    {"KING", 4, false},
};

constexpr int kTape[kTapeN] = {0, 2, 4};

struct Art {
    gs::Image felt;
    gs::Image card;
    gs::Image pip;
    gs::Image reel;
    gs::Image slot;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace solitairetape
