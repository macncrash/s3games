// S3 CAUSEWAY DAWN sprites. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace cdawn {

enum Pal {
    PAL_HUD = 0,
    PAL_YOU = 1,
    PAL_POT = 2,
    PAL_FLAME = 3,
    PAL_SEA = 4,
    PAL_OK = 5,
    PAL_ALERT = 6,
    PAL_POST = 7
};

struct Art {
    gs::Mipped keeper[2];
    gs::Mipped pot;
    gs::Mipped flame;
    gs::Mipped wick;
    gs::Mipped post;
    gs::Mipped gull;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace cdawn
