// Cards, clock, and the system font, drawn into VRAM at boot. No asset files.
#pragma once

#include "console/vdp.h"

namespace memchime {

constexpr int COLS = 4;
constexpr int ROWS = 2;
constexpr int CARDS = COLS * ROWS;
constexpr int FACES = 4;  // 0 is the bell pair. Only that pair can chime.

constexpr int PAL_CARD = 1;
constexpr int PAL_CREAM = 2;
constexpr int PAL_GOLD = 3;
constexpr int PAL_DIM = 4;
constexpr int PAL_LEAF = 5;

struct Art {
    gs::Image back;
    gs::Image face[FACES];
    gs::Image cursor;
    gs::Image tower;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace memchime
