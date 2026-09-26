// S3 BREACH sprites and tiles. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace breach {

enum Pal {
    PAL_HUD = 0,
    PAL_STONE = 1,
    PAL_WOOD = 2,
    PAL_CLOTH = 3,
    PAL_PLAYER = 4,
    PAL_GUARD = 5,
    PAL_FIRE = 6,
    PAL_NIGHT = 7,
    PAL_CARPET = 8,
    PAL_FX = 9,
    PAL_DOOR = 10
};

struct Art {
    gs::Mipped stand, runA, runB, air, duck;
    gs::Mipped guard[3];
    gs::Mipped banner[2];
    gs::Mipped standBase;
    gs::Mipped column;
    gs::Mipped window;
    gs::Mipped sconce;
    gs::Mipped flame[2];
    gs::Mipped crate;
    gs::Mipped beam;
    gs::Mipped chain;
    gs::Mipped door;
    gs::Mipped grate;
    gs::Mipped tapestry;
    gs::Mipped dais;
    gs::Mipped star;
    gs::Mipped dust;
    gs::Mipped shadow;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace breach
