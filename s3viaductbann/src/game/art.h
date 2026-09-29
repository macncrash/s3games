// S3 VIADUCT BANN sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace viaductbann {

enum Pal {
    PAL_HUD = 0,
    PAL_STONE = 1,
    PAL_COAT = 2,
    PAL_BANNER = 3,
    PAL_LOOK = 4,
    PAL_GORGE = 5,
    PAL_MIST = 6,
    PAL_LAMP = 7
};

struct Art {
    gs::Mipped stand, walkA, walkB, leap;
    gs::Mipped look[2];
    gs::Mipped banner, bannerB;
    gs::Mipped ashlar;
    gs::Mipped pier;
    gs::Mipped arch;
    gs::Mipped mist;
    gs::Mipped lamp;
    gs::Mipped abutment;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace viaductbann
