// S3 SPAN COLUMN sprites. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace scol {

enum Pal {
    PAL_HUD = 0,
    PAL_AMBER = 1,
    PAL_ALERT = 2,
    PAL_GOOD = 3,
    PAL_HAND = 4,
    PAL_TRUCK = 5,
    PAL_BEAM = 6,
    PAL_STONE = 7,
    PAL_FX = 8,
    PAL_BUOY = 9,
    PAL_CABIN = 10,
    PAL_FIELD = 12
};

struct Art {
    gs::Mipped hand[2];
    gs::Mipped truck;
    gs::Mipped beam;
    gs::Mipped pier;
    gs::Mipped tower;
    gs::Mipped cabin;
    gs::Mipped lamp;
    gs::Mipped buoy;
    gs::Mipped dust;
    gs::Mipped shadow;
    gs::Mipped sun;
    gs::Mipped cloud;
    gs::Mipped trees;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace scol
