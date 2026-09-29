// S3 CHOIR GOLD pictures. Drawn into VRAM at boot. No asset files.
#pragma once
#include "console/gfx.h"

namespace choirgold {

enum Pal {
    PAL_INK = 0,
    PAL_GOLD = 1,
    PAL_CREAM = 2,
    PAL_NAVE = 3,
    PAL_ROBE = 4,
    PAL_NOTE = 5,
    PAL_WAX = 6,
    PAL_ALERT = 7,
    PAL_HINT = 8
};

struct Art {
    int font[96] = {};
    gs::Image nave;
    gs::Image singer[3];
    gs::Image note;
    gs::Image diamond;
    gs::Image bar;
    gs::Image staff;
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace choirgold
