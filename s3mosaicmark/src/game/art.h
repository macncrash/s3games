// Boot-time tesserae and the mark picture. No asset files.
#pragma once

#include "console/vdp.h"

namespace mark {

constexpr int N = 7;
constexpr int CELLS = N * N;
constexpr int INKS = 5;  // 1..5; 0 is bare plaster

constexpr int PAL_TILE = 1;
constexpr int PAL_CREAM = 2;
constexpr int PAL_GOLD = 3;
constexpr int PAL_DIM = 4;
constexpr int PAL_LEAF = 5;

struct Art {
    gs::Image cell[INKS + 1];
    gs::Image picture;
    gs::Image ring;
    int font[96] = {};
    int mark[CELLS] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace mark
