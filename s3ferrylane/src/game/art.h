// S3 FERRY LANE sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace ferrylane {

enum Pal {
    PAL_HUD = 0,
    PAL_AMBER = 1,
    PAL_BAD = 2,
    PAL_GOOD = 3,
    PAL_SHIP = 4,
    PAL_CREW = 5,
    PAL_BUOY = 6,
    PAL_PIER = 7,
    PAL_SKY = 8,
    PAL_FOAM = 9,
    PAL_GULL = 10,
    PAL_ROAD = 12
};

struct Art {
    gs::Mipped ferry;
    gs::Mipped buoy;
    gs::Mipped pier;
    gs::Mipped cloud;
    gs::Mipped gull[2];
    gs::Mipped foam;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace ferrylane
