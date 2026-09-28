// S3 BUNKER BANN sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace bunkerbann {

enum Pal {
    PAL_HUD = 0,
    PAL_CONCRETE = 1,
    PAL_COAT = 2,
    PAL_BANNER = 3,
    PAL_SENTRY = 4,
    PAL_SAND = 5,
    PAL_LAMP = 6,
    PAL_DUSK = 7
};

struct Art {
    gs::Mipped stand, walkA, walkB, shove;
    gs::Mipped sentry[2];
    gs::Mipped banner;
    gs::Mipped staff;
    gs::Mipped bag;
    gs::Mipped slit;
    gs::Mipped wire;
    gs::Mipped lamp;
    gs::Mipped shadow;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace bunkerbann
