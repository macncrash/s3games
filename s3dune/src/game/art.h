// S3 DUNE pictures. Drawn into VRAM at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace dune {

enum Pal {
    PAL_INK = 0,
    PAL_BUGGY = 1,
    PAL_ROCK = 2,
    PAL_TANK = 3,
    PAL_DUST = 4,
    PAL_TITLE = 5,
    PAL_SIGN = 6,
    PAL_BAD = 7,
    PAL_SKY = 8,
    PAL_HOT = 9,
    PAL_GOOD = 10,
    PAL_PALM = 11,
    PAL_ROAD = 12,
    PAL_DAMP = 13,
    PAL_TENT = 14
};

struct Art {
    gs::Mipped buggy[3];
    gs::Mipped shadow;
    gs::Mipped dust;
    gs::Mipped steam;
    gs::Mipped rock;
    gs::Mipped scrub;
    gs::Mipped tank;
    gs::Mipped palm;
    gs::Mipped post;
    gs::Mipped bannerWater;
    gs::Mipped bannerCamp;
    gs::Mipped tent;
    gs::Image title;
    gs::Image sub;
    gs::Image tag;
    gs::Image count[4];
    gs::Image camp;
    gs::Image boiled;
    gs::Image dry;
    gs::Image paused;
    int font[128] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace dune
