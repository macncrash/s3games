// S3 ALLEY CLER sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace acler {

enum Pal {
    PAL_TEXT = 0,
    PAL_BRICK = 1,
    PAL_SWEEP = 2,
    PAL_JUNK = 3,
    PAL_WOOD = 4,
    PAL_LAMP = 5,
    PAL_STONE = 6,
    PAL_GRATE = 7,
    PAL_GOOD = 8,
    PAL_ALERT = 9,
    PAL_NIGHT = 10
};

struct Art {
    gs::Mipped sweep[2];
    gs::Mipped broom;
    gs::Mipped can;
    gs::Mipped paper;
    gs::Mipped crate;
    gs::Mipped bottle;
    gs::Mipped wall;
    gs::Mipped lamp;
    gs::Mipped grate;
    gs::Mipped clock;
    gs::Mipped shadow;
    gs::Mipped puff;
    gs::Mipped glyph[96];
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace acler
