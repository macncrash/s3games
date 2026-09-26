// S3 DEPOT COLUMN sprites. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace dcol {

enum Pal {
    PAL_TEXT = 0,
    PAL_AMBER = 1,
    PAL_ALERT = 2,
    PAL_GOOD = 3,
    PAL_TANKER = 4,
    PAL_TRUCK = 5,
    PAL_DEPOT = 6,
    PAL_BOOM = 7,
    PAL_YARD = 8,
    PAL_SIGN = 9,
    PAL_FX = 10,
    PAL_COAT = 11,
    PAL_ROAD = 12,
    PAL_WHITE = 13,
    PAL_NIGHT = 14
};

struct Art {
    gs::Mipped tanker, truck, pennant;
    gs::Mipped warehouse, door, apron;
    gs::Mipped tank, drums, crates, crane, shack, sign;
    gs::Mipped stripe, lamp, post, block;
    gs::Mipped watch;
    gs::Mipped dust, shadow, cloud, sun;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace dcol
