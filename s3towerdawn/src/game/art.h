// S3 TOWER DAWN sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace tower {

enum Pal {
    PAL_HUD = 0,
    PAL_AMBER = 1,
    PAL_RED = 2,
    PAL_STONE = 4,
    PAL_KEEPER = 5,
    PAL_FLAME = 6,
    PAL_IRON = 7,
    PAL_MOON = 8
};

struct Art {
    gs::Mipped tower;
    gs::Mipped keeper;
    gs::Mipped brazier;
    gs::Mipped flame[3];
    gs::Mipped moon;
    gs::Mipped star;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace tower
