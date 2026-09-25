// S3 DRIFT pictures. Drawn into VRAM at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace drift {

enum Pal {
    PAL_INK = 0,
    PAL_CAR = 1,
    PAL_WALL = 2,
    PAL_LAMP = 3,
    PAL_SMOKE = 4,
    PAL_TITLE = 5,
    PAL_BANNER = 6,
    PAL_BAD = 7,
    PAL_SKY = 8,
    PAL_HOT = 9,
    PAL_GOOD = 10,
    PAL_ROAD = 12
};

struct Art {
    gs::Mipped car[7];
    gs::Mipped barrier;
    gs::Mipped lamp;
    gs::Mipped smoke;
    gs::Mipped shadow;
    gs::Mipped bannerSlide;
    gs::Mipped bannerFinish;
    gs::Image title;
    gs::Image sub;
    gs::Image tag;
    gs::Image count[4];
    gs::Image walled;
    gs::Image scored;
    gs::Image shortSlide;
    gs::Image paused;
    int font[128] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace drift
