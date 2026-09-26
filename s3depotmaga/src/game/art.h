// S3 DEPOT MAGA pictures. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace depotmaga {

enum Pal {
    PAL_HUD = 0,
    PAL_AMBER = 1,
    PAL_RED = 2,
    PAL_GREEN = 3,
    PAL_LIFT = 4,
    PAL_SHUNT = 5,
    PAL_BRICK = 6,
    PAL_WOOD = 7,
    PAL_METAL = 8,
    PAL_FX = 9,
    PAL_SOOT = 10,
    PAL_YARD = 12
};

struct Art {
    gs::Mipped lift[2];
    gs::Mipped shunt[2];
    gs::Mipped fallen;
    gs::Mipped crate;
    gs::Mipped pile;
    gs::Mipped boxcar;
    gs::Mipped crane;
    gs::Mipped tower;
    gs::Mipped drum;
    gs::Mipped sack;
    gs::Mipped lamp;
    gs::Mipped jamb;
    gs::Mipped sign;
    gs::Mipped clock;
    gs::Mipped rifle;
    gs::Mipped sight;
    gs::Mipped round;
    gs::Mipped spent;
    gs::Mipped flash;
    gs::Mipped spark;
    gs::Mipped glow;
    gs::Mipped chevron;
    gs::Mipped sill;
    gs::Mipped steam;
    gs::Mipped sun;
    gs::Mipped shadow;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace depotmaga
