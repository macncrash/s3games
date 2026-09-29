// S3 SUB LOCK sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace sublock {

enum Pal : int {
    PAL_HUD = 0,
    PAL_SUB = 1,
    PAL_RIVAL = 2,
    PAL_ROCK = 3,
    PAL_GATE = 4,
    PAL_LAMP = 5,
    PAL_BUB = 6,
    PAL_KELP = 7,
    PAL_WIN = 8,
    PAL_ALERT = 9,
    PAL_GOLD = 10,
    PAL_DEEP = 11,
};

struct Art {
    gs::Mipped sub;
    gs::Mipped prop;
    gs::Mipped gate;
    gs::Mipped rock;
    gs::Mipped kelp;
    gs::Mipped lamp;
    gs::Mipped bub;
    gs::Mipped title;
    gs::Mipped clear;
    gs::Mipped fail;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace sublock
