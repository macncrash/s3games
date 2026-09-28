// Boot-time mosaic pictures and the system font. No asset files.
#pragma once

#include "console/vdp.h"

namespace mosaicseven {

constexpr int COLS = 3;
constexpr int ROWS = 3;
constexpr int CELLS = COLS * ROWS;
constexpr int TILE = 32;
constexpr int GAP = 3;
constexpr int PITCH = TILE + GAP;
constexpr int BOARD = COLS * PITCH - GAP;
constexpr int FRAME = 5;
constexpr int OX = 36;
constexpr int OY = 52;
constexpr int PIC = COLS * TILE;
constexpr int THUMB = 64;
constexpr int THUMB_X = 196;
constexpr int PICTURES = 3;
constexpr int BLANK = CELLS - 1;
constexpr int RACE = 7;

constexpr int PAL_CREAM = 0;
constexpr int PAL_GOLD = 1;
constexpr int PAL_GREEN = 2;
constexpr int PAL_DIM = 3;
constexpr int PAL_MOSAIC = 4;
constexpr int PAL_WOOD = 5;
constexpr int PAL_RIVAL = 6;

struct Art {
    gs::Image tile[PICTURES][CELLS];
    gs::Image thumb[PICTURES];
    gs::Image frame;
    gs::Image pip;
    gs::Image pipOn;
    gs::Image pipThem;
    gs::Image arrow[4];
    gs::Image title;
    gs::Image sub;
    gs::Image prompt;
    int font[96] = {};
    const char* name[PICTURES] = {"SUN COURT", "BLUE GATE", "RED ARCH"};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace mosaicseven
