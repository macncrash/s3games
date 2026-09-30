// Pictures drawn at boot. Nothing is loaded from a file.
//
// The tape names three beds, in order: CROWN, LIP, CHEEK.
// WASH, RINSE and DUST are the same width and are not the tape.
#pragma once

#include "console/gfx.h"
#include "console/vdp.h"

namespace masktape {

enum Pal {
    PAL_INK = 0,
    PAL_GOLD = 1,
    PAL_CLAY = 2,
    PAL_WOOD = 3,
    PAL_BRUSH = 4,
    PAL_SLIP = 5,
    PAL_PAPER = 6,
    PAL_DRAWER = 7,
    PAL_TITLE = 8,
    PAL_BAD = 9,
    PAL_DIM = 10,
    PAL_WALL = 11,
    PAL_MARK = 12,
    PAL_WAX = 13,
    PAL_HOOK = 14,
    PAL_DECOY = 15
};

struct Art {
    int font[96] = {};
    gs::Image mask;
    gs::Image brush;
    gs::Image rail;
    gs::Image paper;
    gs::Image drawer;
    gs::Image slip;
    gs::Image hook;
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace masktape
