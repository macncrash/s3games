// S3 BEDSTAPE pictures. Drawn at boot. No asset files.
#pragma once

#include "console/gfx.h"
#include "console/vdp.h"

namespace bedstape {

constexpr int kHerbs = 4;
constexpr int kTapeN = 3;

// BASIL, DILL, MINT go in the drawer. SAGE pays like DILL and stays out.
struct Herb {
    const char* name;
    int pay;
    bool decoy;
};

constexpr Herb kHerb[kHerbs] = {
    {"BASIL", 3, false},
    {"DILL", 5, false},
    {"MINT", 4, false},
    {"SAGE", 5, true},
};

constexpr int kTape[kTapeN] = {0, 1, 2};

// Ripe order. The two SAGE calls are not on the tape.
constexpr int kDeckN = 5;
constexpr int kDeck[kDeckN] = {3, 0, 1, 3, 2};

enum Pal {
    PAL_HUD = 0,
    PAL_GOLD = 1,
    PAL_WARN = 2,
    PAL_GOOD = 3,
    PAL_DIM = 4,
    PAL_WOOD = 5,
    PAL_SOIL = 6,
    PAL_LEAF = 7,
    PAL_SAGE = 8,
    PAL_MAN = 9,
    PAL_WATER = 10,
    PAL_YARD = 11,
    PAL_PATH = 12,
    PAL_CAN = 13,
    PAL_SUN = 14,
    PAL_DRAWER = 15
};

struct Art {
    int font[96] = {};
    int grass = 0;
    int path = 0;
    gs::Mipped title;
    gs::Mipped sub;
    gs::Mipped win;
    gs::Mipped late;
    gs::Mipped bed;
    gs::Mipped soil;
    gs::Mipped herb[kHerbs];
    gs::Mipped man[2];
    gs::Mipped can;
    gs::Mipped drop;
    gs::Mipped shade;
    gs::Mipped slip;
    gs::Mipped drawer;
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace bedstape
