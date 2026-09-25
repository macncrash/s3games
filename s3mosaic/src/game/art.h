// Boot-time mosaic pictures and the system font. No asset files.
#pragma once

#include "console/vdp.h"

namespace mosaic {

constexpr int COLS = 4;
constexpr int ROWS = 4;
constexpr int CELLS = COLS * ROWS;
constexpr int TILE = 36;
constexpr int GAP = 3;
constexpr int PITCH = TILE + GAP;
constexpr int BOARD = COLS * PITCH - GAP;
constexpr int FRAME = 6;
constexpr int OX = 28;
constexpr int OY = 40;
constexpr int PIC = COLS * TILE;
constexpr int THUMB = 80;
constexpr int THUMB_X = 200;
constexpr int PICTURES = 3;

constexpr int PAL_CREAM = 0;
constexpr int PAL_COAST = 1;
constexpr int PAL_GARDEN = 2;
constexpr int PAL_NIGHT = 3;
constexpr int PAL_GOLD = 4;
constexpr int PAL_GREEN = 5;
constexpr int PAL_DIM = 6;
constexpr int PAL_CORAL = 7;
constexpr int PAL_WOOD = 8;

constexpr int BLANK = CELLS - 1;

struct Art {
    gs::Image tile[PICTURES][CELLS];
    gs::Image thumb[PICTURES];
    gs::Image frame;
    gs::Image arrow[4];
    gs::Image title;
    gs::Image sub;
    gs::Image tag;
    gs::Image prompt;
    gs::Image goal;
    int font[96] = {};
    int pal[PICTURES] = {PAL_COAST, PAL_GARDEN, PAL_NIGHT};
    int depth[PICTURES] = {10, 18, 26};
    const char* name[PICTURES] = {"COAST LIGHT", "GARDEN GATE", "NIGHT MARKET"};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace mosaic
