// Boot-time tesserae and the bell picture. No asset files.
#pragma once

#include "console/vdp.h"

namespace mosaicbell {

constexpr int COLS = 5;
constexpr int ROWS = 6;
constexpr int CELLS = COLS * ROWS;
constexpr int INKS = 4;  // 1..4; 0 is bare plaster

constexpr int PAL_TILE = 1;
constexpr int PAL_CREAM = 2;
constexpr int PAL_GOLD = 3;
constexpr int PAL_DIM = 4;
constexpr int PAL_LEAF = 5;

struct Art {
    gs::Image cell[INKS + 1];
    gs::Image picture;
    gs::Image ring;
    gs::Image bell;
    int font[96] = {};
    int mark[CELLS] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace mosaicbell
