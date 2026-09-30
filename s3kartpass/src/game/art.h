// S3 KART PASS pictures. Drawn into VRAM at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace kartpass {

enum Pal {
    PAL_INK = 0,
    PAL_KART = 1,
    PAL_PEAK = 2,
    PAL_BALE = 3,
    PAL_SPRAY = 4,
    PAL_TITLE = 5,
    PAL_GATE = 6,
    PAL_BAD = 7,
    PAL_SKY = 8,
    PAL_HOT = 9,
    PAL_GOOD = 10,
    PAL_ROAD = 12
};

struct Art {
    gs::Mipped kart[5];
    gs::Mipped peak;
    gs::Mipped bale;
    gs::Mipped spray;
    gs::Mipped arch;
    gs::Image title;
    gs::Image sub;
    gs::Image cleared;
    gs::Image missed;
    gs::Image late;
    gs::Image paused;
    int font[128] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace kartpass
