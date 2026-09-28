// S3 PLOW KILO pictures. Drawn into sprite ROM at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace plow {

enum Pal {
    PAL_INK = 0,
    PAL_GOLD = 1,
    PAL_PLOW = 2,
    PAL_HORSE = 3,
    PAL_WHEEL = 4,
    PAL_WAGON = 5,
    PAL_TYRE = 6,
    PAL_FIELD = 7,
    PAL_MARK = 8,
    PAL_DIRT = 12
};

struct Art {
    gs::Image glyph[96];
    gs::Mipped plow;
    gs::Mipped horse;
    gs::Mipped wheel[3];
    gs::Mipped wagon;
    gs::Mipped tyre;
    gs::Mipped stake;
    gs::Mipped tree;
    gs::Mipped barn;
    gs::Mipped flag;
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace plow
