// Boot-time tesserae, the tape, and the drawer. No asset files.
#pragma once

#include "console/vdp.h"

namespace tape {

constexpr int N = 8;

constexpr int PAL_TILE = 1;
constexpr int PAL_CREAM = 2;
constexpr int PAL_GOLD = 3;
constexpr int PAL_DIM = 4;
constexpr int PAL_LEAF = 5;

struct Art {
    gs::Image piece[N];
    gs::Image drawer;
    gs::Image tape;
    gs::Image cursor;
    int font[96] = {};
    // Target order of tesserae. The drawer must become this sequence.
    int order[N] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace tape
