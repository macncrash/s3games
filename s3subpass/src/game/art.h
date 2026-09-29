// S3 SUBPASS sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace subpass {

enum Pal {
    PAL_HUD = 0,
    PAL_SUB = 1,
    PAL_CREW = 2,
    PAL_ROCK = 3,
    PAL_MINE = 4,
    PAL_KELP = 5,
    PAL_GATE = 6,
    PAL_FX = 7,
    PAL_FISH = 8
};

struct Art {
    gs::Mipped sub;
    gs::Mipped crew;
    gs::Mipped rock;
    gs::Mipped mine;
    gs::Mipped kelp;
    gs::Mipped gate;
    gs::Mipped bubble;
    gs::Mipped fish;
    gs::Mipped banner;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace subpass
