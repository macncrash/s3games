// S3 FOUNDRY BANN sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace foundry {

enum Pal {
    PAL_HUD = 0,
    PAL_BRICK = 1,
    PAL_YOU = 2,
    PAL_IRON = 3,
    PAL_MELT = 4,
    PAL_BANNER = 5,
    PAL_SOOT = 6,
    PAL_OK = 7,
    PAL_ALERT = 8
};

struct Art {
    gs::Mipped worker[2];
    gs::Mipped banner[2];
    gs::Mipped ingot;
    gs::Mipped pour;
    gs::Mipped ladle;
    gs::Mipped arch;
    gs::Mipped stack;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace foundry
