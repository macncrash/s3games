// S3 TOWER RELIEF sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace tower {

enum Pal {
    PAL_HUD = 0,
    PAL_STONE = 1,
    PAL_YOU = 2,
    PAL_CLIMB = 3,
    PAL_SHIELD = 4,
    PAL_FX = 5,
    PAL_BELL = 6,
    PAL_ALERT = 7,
    PAL_OK = 8
};

struct Art {
    gs::Mipped tower, flag, sentinel, stone, flash;
    gs::Mipped climb[2], hook[2], shield[2];
    gs::Mipped bell, rope;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace tower
