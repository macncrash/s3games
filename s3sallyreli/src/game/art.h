// S3 SALLY RELIEF sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace sally {

enum Pal {
    PAL_HUD = 0,
    PAL_WALL = 1,
    PAL_YOU = 2,
    PAL_RAID = 3,
    PAL_SHIELD = 4,
    PAL_FX = 5,
    PAL_BELL = 6,
    PAL_ALERT = 7,
    PAL_OK = 8,
    PAL_FIELD = 9
};

struct Art {
    gs::Mipped wall, arch, banner;
    gs::Mipped you[2], pike, pistol;
    gs::Mipped raider[2], runner[2], shield[2];
    gs::Mipped bell, flash, puff;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace sally
