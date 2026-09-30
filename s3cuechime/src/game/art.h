// Pictures drawn at boot. No asset files.
#pragma once

#include "console/gfx.h"
#include "console/vdp.h"

namespace cuechime {

enum Pal {
    PAL_HUD = 0,
    PAL_WOOD = 1,
    PAL_CLOTH = 2,
    PAL_CUE = 3,
    PAL_BALL = 4,
    PAL_CLOCK = 5,
    PAL_PLAYER = 6,
    PAL_POCKET = 7
};

struct Art {
    gs::Mipped table;
    gs::Mipped cue;
    gs::Mipped cueBall;
    gs::Mipped object;
    gs::Mipped clock;
    gs::Mipped hand;
    gs::Mipped player;
    gs::Mipped pocket;
    gs::Image title;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace cuechime
