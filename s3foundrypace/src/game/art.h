// S3 FOUNDRY PACE pictures. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace foundrypace {

enum Pal {
    PAL_TEXT = 0,
    PAL_EMBER = 1,
    PAL_ALERT = 2,
    PAL_GOOD = 3,
    PAL_IRON = 4,
    PAL_FIGURE = 5,
    PAL_HOLD = 6,
    PAL_LIVE = 7,
    PAL_SOOT = 8,
    PAL_FX = 9,
    PAL_GLOW = 10,
    PAL_BRICK = 11,
    PAL_ROAD = 12,
    PAL_STACK = 13,
    PAL_SLAG = 14
};

struct Art {
    gs::Mipped smith[2];
    gs::Mipped fallen;
    gs::Mipped crucible;
    gs::Mipped ingot;
    gs::Mipped furnace;
    gs::Mipped flame[2];
    gs::Mipped chimney;
    gs::Mipped stake;
    gs::Mipped bead;
    gs::Mipped iron;
    gs::Mipped post;
    gs::Mipped bellows[2];
    gs::Mipped lip;
    gs::Mipped pip;
    gs::Mipped flash;
    gs::Mipped dust;
    gs::Mipped shadow;
    gs::Mipped stripe;
    gs::Mipped hood;
    gs::Mipped spark;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace foundrypace
