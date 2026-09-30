// S3 DRUMMARK sprites. Drawn into VDP RAM at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace drummark {

enum Pal {
    PAL_HUD = 0,
    PAL_GOLD = 1,
    PAL_RED = 2,
    PAL_GREEN = 3,
    PAL_DRUM = 4,
    PAL_PLAYER = 5,
    PAL_STAGE = 6,
    PAL_LAMP = 7,
    PAL_FX = 8
};

struct Art {
    gs::Mipped drum;
    gs::Mipped stickUp;
    gs::Mipped stickDown;
    gs::Mipped player;
    gs::Mipped lamp;
    gs::Mipped curtain;
    gs::Mipped pip;
    gs::Mipped pipOn;
    gs::Mipped beater;
    gs::Mipped card;
    gs::Mipped stamp;
    gs::Mipped stool;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace drummark
