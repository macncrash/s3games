// S3 MILLDAWN pictures. Drawn into sprite ROM at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace mill {

enum Pal {
    PAL_HUD = 0,
    PAL_MILL = 1,
    PAL_MAN = 2,
    PAL_FLAME = 3,
    PAL_YARD = 4,
    PAL_MOON = 5
};

struct Art {
    gs::Mipped mill[2];
    gs::Mipped man[2];
    gs::Mipped flame[2];
    gs::Mipped stake;
    gs::Mipped moon;
    gs::Mipped star;
    gs::Mipped gust;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace mill
