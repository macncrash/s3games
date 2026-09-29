// S3 SUB BOX sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace subbox {

enum Pal {
    PAL_HUD = 0,
    PAL_SUB = 1,
    PAL_BOX = 2,
    PAL_ROCK = 3,
    PAL_BUB = 4,
    PAL_KELP = 5,
    PAL_WATER = 6
};

struct Art {
    gs::Mipped sub;
    gs::Mipped box;
    gs::Mipped rock;
    gs::Mipped kelp;
    gs::Mipped bubble;
    gs::Mipped lamp;
    int font[96] = {};
    int waterTile = 1;
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace subbox
