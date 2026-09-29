// S3 MAILVAN BOOM sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace boom {

enum Pal {
    PAL_HUD = 0,
    PAL_CREAM = 1,
    PAL_BAD = 2,
    PAL_GOOD = 3,
    PAL_VAN = 4,
    PAL_RIVAL = 5,
    PAL_BOOM = 6,
    PAL_POST = 7,
    PAL_ROAD = 12
};

struct Art {
    gs::Mipped hood;
    gs::Mipped rival;
    gs::Mipped arm;
    gs::Mipped armUp;
    gs::Mipped post;
    gs::Mipped bollard;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace boom
