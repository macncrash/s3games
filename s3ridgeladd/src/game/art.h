// S3 RIDGE LADD sprites. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace rladd {

enum Pal {
    PAL_HUD = 0,
    PAL_AMBER = 1,
    PAL_ALERT = 2,
    PAL_GOOD = 3,
    PAL_YOU = 4,
    PAL_WOOD = 5,
    PAL_STONE = 6,
    PAL_FX = 7,
    PAL_MOUNT = 8,
    PAL_FIELD = 12
};

struct Art {
    gs::Mipped walk[2];
    gs::Mipped jump;
    gs::Mipped climb;
    gs::Mipped ladder;
    gs::Mipped rag;
    gs::Mipped cliff;
    gs::Mipped post;
    gs::Mipped tooth;
    gs::Mipped stone[2];
    gs::Mipped dust;
    gs::Mipped shadow;
    gs::Mipped sun;
    gs::Mipped cloud;
    gs::Mipped peak[2];
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace rladd
