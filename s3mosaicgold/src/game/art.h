// Boot-time mosaic and the system font. No asset files.
#pragma once

#include "console/vdp.h"

namespace mosaicgold {

constexpr int COLS = 4;
constexpr int ROWS = 4;
constexpr int CELLS = COLS * ROWS;
constexpr int TILE = 36;
constexpr int GAP = 3;
constexpr int PITCH = TILE + GAP;
constexpr int BOARD = COLS * PITCH - GAP;
constexpr int FRAME = 6;
constexpr int OX = 28;
constexpr int OY = 36;
constexpr int PIC = COLS * TILE;
constexpr int THUMB = 72;
constexpr int THUMB_X = 208;
constexpr int BLANK = CELLS - 1;
constexpr int LINE = 20;

constexpr int PAL_CREAM = 0;
constexpr int PAL_GOLD = 1;
constexpr int PAL_GREEN = 2;
constexpr int PAL_DIM = 3;
constexpr int PAL_MOSAIC = 4;
constexpr int PAL_WOOD = 5;

struct Art {
    gs::Image tile[CELLS];
    gs::Image thumb;
    gs::Image frame;
    gs::Image arrow[4];
    gs::Image title;
    gs::Image sub;
    gs::Image tag;
    gs::Image prompt;
    int font[96] = {};
    // Tile ids 0..14. The blank is not a score.
    uint8_t gold[CELLS] = {0, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 0, 0, 1, 0, 0};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace mosaicgold
