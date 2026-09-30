// S3 BEACON RELIEF sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace beacon {

enum Pal {
    PAL_HUD = 0,
    PAL_ALERT = 1,
    PAL_OK = 2,
    PAL_BELL = 3,
    PAL_KEEP = 4,
    PAL_WRECK = 5,
    PAL_DOUSE = 6,
    PAL_ROCK = 7,
    PAL_LAMP = 8,
    PAL_FX = 9
};

struct Art {
    gs::Mipped house, flame, glass, keeper, beam;
    gs::Mipped wreck[2], douse[2];
    gs::Mipped flare, bell, post, rock;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace beacon
