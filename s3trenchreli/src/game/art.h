// S3 TRENCH RELIEF sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace trench {

enum Pal {
    PAL_HUD = 0,
    PAL_BAG = 1,
    PAL_YOU = 2,
    PAL_FOE = 3,
    PAL_PLATE = 4,
    PAL_FX = 5,
    PAL_BELL = 6,
    PAL_ALERT = 7,
    PAL_OK = 8
};

struct Art {
    gs::Mipped bags, boards, wire, rifle, sight, flash;
    gs::Mipped climber[2], runner[2], shield[2];
    gs::Mipped soldier;
    gs::Mipped bell, stake;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace trench
