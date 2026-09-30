// Pictures drawn at boot. No asset files.
#pragma once

#include "console/gfx.h"
#include "console/vdp.h"

namespace cuebell {

enum Pal {
    PAL_HUD = 0,
    PAL_WOOD = 1,
    PAL_CLOTH = 2,
    PAL_CUE = 3,
    PAL_BALL = 4,
    PAL_BELL = 5,
    PAL_PLAYER = 6,
    PAL_DEAD = 7
};

struct Art {
    gs::Mipped table;
    gs::Mipped cue;
    gs::Mipped ball;
    gs::Mipped bell;
    gs::Mipped player;
    gs::Mipped mark;
    gs::Image title;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace cuebell
