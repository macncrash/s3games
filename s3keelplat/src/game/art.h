// S3 KEELPLAT sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace keelplat {

enum Pal : int {
    PAL_HUD = 0,
    PAL_HULL = 1,
    PAL_SAIL = 2,
    PAL_QUAY = 3,
    PAL_WATER = 4,
    PAL_CREW = 5,
    PAL_POST = 6,
    PAL_WIN = 7,
    PAL_ALERT = 8,
    PAL_BANNER = 9,
    PAL_GULL = 10,
};

struct Art {
    gs::Mipped hull;
    gs::Mipped sail;
    gs::Mipped quay;
    gs::Mipped post;
    gs::Mipped wave;
    gs::Mipped gull;
    gs::Mipped rival;
    gs::Mipped title;
    gs::Mipped level;
    gs::Mipped late;
    gs::Mipped paused;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace keelplat
