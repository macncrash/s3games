// S3 TOWER BANN sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace towerbann {

enum Pal {
    PAL_HUD = 0,
    PAL_STONE = 1,
    PAL_HERO = 2,
    PAL_BANNER = 3,
    PAL_CROW = 4,
    PAL_DOOR = 5,
    PAL_AMBER = 6,
    PAL_NIGHT = 7
};

struct Art {
    gs::Mipped stand, walkA, walkB, jump;
    gs::Mipped crow[2];
    gs::Mipped banner;
    gs::Mipped pole;
    gs::Mipped door;
    gs::Mipped ledge;
    gs::Mipped wall;
    gs::Mipped slit;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace towerbann
