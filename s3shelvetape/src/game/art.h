// S3 SHELVETAPE pictures. Drawn at boot. No asset files.
#pragma once

#include "console/gfx.h"
#include "console/vdp.h"

namespace shelvetape {

enum Pal {
    PAL_TEXT = 0,
    PAL_AMBER = 1,
    PAL_RED = 2,
    PAL_GREEN = 3,
    PAL_ROOM = 4,
    PAL_PAPER = 5,
    PAL_WOOD = 6,
    PAL_FOLIO = 7,
    PAL_ATLAS = 8,
    PAL_PRIMER = 9,
    PAL_LEDGER = 10,
    PAL_INK = 11
};

constexpr int kRows = 4;
constexpr int kTapeN = 3;

// The tape is the first three. LEDGER pays the same as ATLAS and stays out.
struct Spine {
    const char* name;
    int pay;
    bool decoy;
    int pal;
};

constexpr Spine kSpine[kRows] = {
    {"FOLIO", 4, false, PAL_FOLIO},
    {"ATLAS", 7, false, PAL_ATLAS},
    {"PRIMER", 3, false, PAL_PRIMER},
    {"LEDGER", 7, true, PAL_LEDGER},
};

constexpr int kTape[kTapeN] = {0, 1, 2};

struct Art {
    gs::Mipped book[kRows];
    gs::Mipped cart;
    gs::Mipped hand;
    gs::Mipped solid;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace shelvetape
