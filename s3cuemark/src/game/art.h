// Pictures drawn at boot. The gold chalk spot is the only finished mark.
#pragma once

#include "console/gfx.h"
#include "console/vdp.h"

namespace cuemark {

enum Pal {
    PAL_HUD = 0,
    PAL_WOOD = 1,
    PAL_CLOTH = 2,
    PAL_CUE = 3,
    PAL_BALL = 4,
    PAL_GOLD = 5,
    PAL_PLAYER = 6,
    PAL_CHALK = 7
};

struct Art {
    gs::Mipped table;
    gs::Mipped cue;
    gs::Mipped ball;
    gs::Mipped spot;
    gs::Mipped chalk;
    gs::Mipped player;
    gs::Image title;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace cuemark
