// S3 METRO PASS pictures. Drawn into VRAM at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace metropass {

enum Pal {
    PAL_INK = 0,
    PAL_CAR = 1,
    PAL_PEAK = 2,
    PAL_ROCK = 3,
    PAL_SNOW = 4,
    PAL_TITLE = 5,
    PAL_MOUTH = 6,
    PAL_POLE = 7,
    PAL_SKY = 8,
    PAL_HOT = 9,
    PAL_GOOD = 10,
    PAL_BAD = 11,
    PAL_ROAD = 12
};

struct Art {
    gs::Mipped car[3];
    gs::Mipped peak;
    gs::Mipped rock;
    gs::Mipped snow;
    gs::Mipped mouth;
    gs::Mipped pole;
    gs::Image title;
    gs::Image sub;
    gs::Image cleared;
    gs::Image derail;
    gs::Image late;
    gs::Image held;
    int font[128] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace metropass
