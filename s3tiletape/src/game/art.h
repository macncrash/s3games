// Boot-time short tiles, the tape, and the drawer. No asset files.
#pragma once

#include "console/vdp.h"

namespace tiletape {

constexpr int N = 4;

constexpr int PAL_TILE = 1;
constexpr int PAL_CREAM = 2;
constexpr int PAL_GOLD = 3;
constexpr int PAL_DIM = 4;
constexpr int PAL_LEAF = 5;

struct Art {
    gs::Image tile[N];
    gs::Image drawer;
    gs::Image tape;
    gs::Image cursor;
    gs::Image lift;
    int font[96] = {};
    // The picture on the tape. The drawer has to become this, slot for slot.
    int order[N] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace tiletape
