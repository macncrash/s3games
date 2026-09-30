// S3 KARTSLIP sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace slip {

enum Pal {
    PAL_HUD = 0,
    PAL_YOU = 1,
    PAL_CREW = 2,
    PAL_WOOD = 3,
    PAL_BOAT = 4,
    PAL_POST = 5,
    PAL_FX = 6,
    PAL_FIELD = 12
};

struct Art {
    gs::Mipped kart;
    gs::Mipped crate;
    gs::Mipped post;
    gs::Mipped boat;
    gs::Mipped buoy;
    gs::Mipped splash;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace slip
