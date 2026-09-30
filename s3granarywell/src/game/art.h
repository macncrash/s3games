// S3 GRANARY WELL sprites. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace granary {

enum Pal {
    PAL_HUD = 0,
    PAL_STONE = 1,
    PAL_WELL = 2,
    PAL_GUARD = 3,
    PAL_GRANARY = 4,
    PAL_FX = 5,
    PAL_YARD = 12
};

struct Art {
    gs::Mipped granary;
    gs::Mipped well;
    gs::Mipped wellFall;
    gs::Mipped guard;
    gs::Mipped stone;
    gs::Mipped stave;
    gs::Mipped puff;
    gs::Mipped bucket;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace granary
