// S3 LUGE PASS pictures. Drawn into VRAM at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace luge {

enum Pal {
    PAL_INK = 0,
    PAL_RIDER = 1,
    PAL_PINE = 2,
    PAL_ROCK = 3,
    PAL_SPRAY = 4,
    PAL_TITLE = 5,
    PAL_BANNER = 6,
    PAL_BAD = 7,
    PAL_SKY = 8,
    PAL_HOT = 9,
    PAL_GOOD = 10,
    PAL_ROAD = 12
};

struct Art {
    gs::Mipped rider[5];
    gs::Mipped pine;
    gs::Mipped rock;
    gs::Mipped spray;
    gs::Mipped shadow;
    gs::Mipped bannerOpen;
    gs::Mipped bannerShut;
    gs::Image title;
    gs::Image sub;
    gs::Image count[4];
    gs::Image cleared;
    gs::Image buried;
    gs::Image late;
    gs::Image paused;
    int font[128] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace luge
