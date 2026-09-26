// S3 YARD MAGA pictures. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace yardmaga {

enum Pal {
    PAL_HUD = 0,
    PAL_AMBER = 1,
    PAL_RED = 2,
    PAL_GREEN = 3,
    PAL_CUT = 4,
    PAL_HAND = 5,
    PAL_WOOD = 6,
    PAL_IRON = 7,
    PAL_BRICK = 8,
    PAL_FX = 9,
    PAL_NIGHT = 10,
    PAL_YARD = 12
};

struct Art {
    gs::Mipped cutter[2];
    gs::Mipped hand[2];
    gs::Mipped fallen;
    gs::Mipped shed;
    gs::Mipped boxcar;
    gs::Mipped crane;
    gs::Mipped tower;
    gs::Mipped pallet;
    gs::Mipped drum;
    gs::Mipped lamp;
    gs::Mipped pool;
    gs::Mipped chevron;
    gs::Mipped jamb;
    gs::Mipped sill;
    gs::Mipped rifle;
    gs::Mipped sight;
    gs::Mipped round;
    gs::Mipped spent;
    gs::Mipped flash;
    gs::Mipped spark;
    gs::Mipped glow;
    gs::Mipped sun;
    gs::Mipped star;
    gs::Mipped shadow;
    gs::Mipped steam;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace yardmaga
