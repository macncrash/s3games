// S3 ALLEY BANN sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace alleybann {

enum Pal {
    PAL_HUD = 0,
    PAL_BRICK = 1,
    PAL_COAT = 2,
    PAL_BANNER = 3,
    PAL_WATCH = 4,
    PAL_WOOD = 5,
    PAL_LAMP = 6,
    PAL_NIGHT = 7
};

struct Art {
    gs::Mipped stand, walkA, walkB, shove;
    gs::Mipped watcher[2];
    gs::Mipped banner;
    gs::Mipped pole;
    gs::Mipped crate;
    gs::Mipped bin;
    gs::Mipped lamp;
    gs::Mipped fire;
    gs::Mipped shadow;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace alleybann
