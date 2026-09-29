// S3 REDOUBT CLER sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace rcler {

enum Pal {
    PAL_TEXT = 0,
    PAL_GOLD = 1,
    PAL_ALERT = 2,
    PAL_GOOD = 3,
    PAL_EARTH = 4,
    PAL_SAPPER = 5,
    PAL_WOOD = 6,
    PAL_IRON = 7,
    PAL_FLAG = 8,
    PAL_WICKER = 9,
    PAL_POWDER = 10,
    PAL_DUST = 11,
    PAL_SHADE = 12
};

struct Art {
    gs::Mipped sapper[2];
    gs::Mipped mattock;
    gs::Mipped spoil;
    gs::Mipped fascine;
    gs::Mipped gabion;
    gs::Mipped stake;
    gs::Mipped keg;
    gs::Mipped wheel;
    gs::Mipped crate;
    gs::Mipped rampart;
    gs::Mipped merlon;
    gs::Mipped flag;
    gs::Mipped embrasure;
    gs::Mipped revet;
    gs::Mipped dust;
    gs::Mipped shadow;
    gs::Mipped scar;
    gs::Mipped glyph[96];
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace rcler
