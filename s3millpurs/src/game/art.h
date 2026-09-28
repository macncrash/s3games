// S3 MILL PURSUIT sprites. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace millp {

enum Pal {
    PAL_TEXT = 0,
    PAL_WHEAT = 1,
    PAL_ALERT = 2,
    PAL_GOOD = 3,
    PAL_THRESH = 4,
    PAL_TRACTOR = 5,
    PAL_MILL = 6,
    PAL_SAIL = 7,
    PAL_STONE = 8,
    PAL_DUST = 9,
    PAL_IRON = 10
};

struct Art {
    gs::Mipped mill, sailA, sailB, thresher, tractor, stone, dust, stalk;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace millp
