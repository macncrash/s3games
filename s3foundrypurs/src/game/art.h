// S3 FOUNDRY PURSUIT sprites. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace foundryp {

enum Pal {
    PAL_TEXT = 0,
    PAL_HEAT = 1,
    PAL_ALERT = 2,
    PAL_GOOD = 3,
    PAL_SLAG = 4,
    PAL_CRUC = 5,
    PAL_FURN = 6,
    PAL_LADLE = 7,
    PAL_SPARK = 8,
    PAL_EMBER = 9,
    PAL_SOOT = 10
};

struct Art {
    gs::Mipped furnace, ladle, slag, crucible, spark, ember, stack;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace foundryp
