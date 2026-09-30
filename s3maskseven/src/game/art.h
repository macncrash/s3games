// Pictures drawn at boot. Nothing is loaded from a file.
#pragma once

#include "console/gfx.h"
#include "console/vdp.h"

namespace maskseven {

enum Pal {
    PAL_INK = 0,
    PAL_GOLD = 1,
    PAL_RIVAL = 2,
    PAL_CLAY = 3,
    PAL_WOOD = 4,
    PAL_BRUSH = 5,
    PAL_FOIL = 6,
    PAL_BENCH = 7,
    PAL_TITLE = 8,
    PAL_BAD = 9,
    PAL_DIM = 10,
    PAL_WALL = 11,
    PAL_YOU = 12,
    PAL_HOOK = 13,
    PAL_SHADE = 14,
    PAL_MARK = 15
};

struct Art {
    int font[96] = {};
    gs::Image yours;
    gs::Image theirs;
    gs::Image brush;
    gs::Image foil;
    gs::Image bar;
    gs::Image hook;
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace maskseven
