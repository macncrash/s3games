// S3 RIDGE WELL sprites. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace rwell {

enum Pal {
    PAL_HUD = 0,
    PAL_AMBER = 1,
    PAL_ALERT = 2,
    PAL_GOOD = 3,
    PAL_YOU = 4,
    PAL_FOE = 5,
    PAL_RAM = 6,
    PAL_STONE = 7,
    PAL_WOOD = 8,
    PAL_FX = 9,
    PAL_MOUNT = 10,
    PAL_BANNER = 11,
    PAL_FIELD = 12
};

struct Art {
    gs::Mipped warden[2];
    gs::Mipped runner[2];
    gs::Mipped club[2];
    gs::Mipped ram[2];
    gs::Mipped well;
    gs::Mipped rubble;
    gs::Mipped crack;
    gs::Mipped bucket;
    gs::Mipped stake;
    gs::Mipped cairn;
    gs::Mipped post;
    gs::Mipped pennant;
    gs::Mipped dust;
    gs::Mipped shock;
    gs::Mipped shadow;
    gs::Mipped sun;
    gs::Mipped cloud;
    gs::Mipped peak[2];
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace rwell
