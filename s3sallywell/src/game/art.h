// S3 SALLY WELL sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace sally {

enum Pal {
    PAL_HUD = 0,
    PAL_STONE = 1,
    PAL_GUARD = 2,
    PAL_RAID = 3,
    PAL_FX = 4,
    PAL_ROAD = 12
};

struct Art {
    gs::Mipped guard;
    gs::Mipped raider;
    gs::Mipped well;
    gs::Mipped arch;
    gs::Mipped torch;
    gs::Mipped stone;
    gs::Mipped spark;
    gs::Mipped shadow;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace sally
