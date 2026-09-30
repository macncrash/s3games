// S3 SALLY DAWN sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace sally {

enum Pal {
    PAL_HUD = 0,
    PAL_AMBER = 1,
    PAL_FLAME = 2,
    PAL_MAN = 3,
    PAL_WOOD = 4,
    PAL_STONE = 5,
    PAL_RED = 6
};

struct Art {
    gs::Mipped wall;
    gs::Mipped moon;
    gs::Mipped star;
    gs::Mipped pot;
    gs::Mipped flame[3];
    gs::Mipped sentry[2];
    gs::Mipped bar;
    gs::Mipped glyph[96];
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace sally
