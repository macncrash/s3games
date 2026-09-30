// S3 HORNSEVEN sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace hornseven {

enum Pal {
    PAL_INK = 0,
    PAL_GOLD = 1,
    PAL_YOU = 2,
    PAL_THEM = 3,
    PAL_WOOD = 4,
    PAL_NOTE = 5,
    PAL_LAMP = 6,
    PAL_ALERT = 7
};

struct Art {
    gs::Mipped player[2];
    gs::Mipped rival[2];
    gs::Mipped curtain;
    gs::Mipped lamp;
    gs::Mipped note;
    gs::Mipped stand;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace hornseven
