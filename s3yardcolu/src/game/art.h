// S3 YARD COLUMN sprites. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace ycol {

enum Pal {
    PAL_TEXT = 0,
    PAL_AMBER = 1,
    PAL_ALERT = 2,
    PAL_GOOD = 3,
    PAL_TRUCK = 4,
    PAL_MULE = 5,
    PAL_HULK = 6,
    PAL_YARD = 7,
    PAL_GANTRY = 8,
    PAL_SIGN = 9,
    PAL_FX = 10,
    PAL_MAGNET = 11,
    PAL_ROAD = 12,
    PAL_DUSK = 13,
    PAL_HAZARD = 14
};

struct Art {
    gs::Mipped truck, mule, hulk, magnet, pennant;
    gs::Mipped post, beam, cable, mark, lamp;
    gs::Mipped scrap, stack, drums, shack, sign, fence, baler;
    gs::Mipped dust, shadow, cloud, sun;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace ycol
