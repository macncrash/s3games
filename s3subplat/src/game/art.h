// S3 SUBPLAT sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace subplat {

enum Pal : int {
    PAL_HUD = 0,
    PAL_SUB = 1,
    PAL_PLAT = 2,
    PAL_KELP = 3,
    PAL_FISH = 4,
    PAL_BUB = 5,
    PAL_RIVAL = 6,
    PAL_WIN = 7,
    PAL_ALERT = 8,
    PAL_BANNER = 9,
    PAL_LIGHT = 10,
};

struct Art {
    gs::Mipped sub;
    gs::Mipped plat;
    gs::Mipped kelp;
    gs::Mipped fish;
    gs::Mipped bub;
    gs::Mipped rival;
    gs::Mipped lamp;
    gs::Mipped title;
    gs::Mipped level;
    gs::Mipped late;
    gs::Mipped paused;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace subplat
