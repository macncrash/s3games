// S3 TRAM LOCK sprites. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace tramlock {

enum Pal {
    PAL_HUD = 0,
    PAL_AMBER = 1,
    PAL_BAD = 2,
    PAL_GOOD = 3,
    PAL_CAB = 4,
    PAL_STONE = 5,
    PAL_GATE = 6,
    PAL_IRON = 7,
    PAL_SIGN = 8,
    PAL_ROAD = 12,
    PAL_CHAMBER = 13
};

struct Art {
    gs::Mipped dash, lever[4], leaf, wall, lamp, wire, capstan, board;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace tramlock
