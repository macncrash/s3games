// S3 FISH MARK sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace fishmark {

enum Pal {
    PAL_HUD = 0,
    PAL_FISH = 1,
    PAL_ANGLER = 2,
    PAL_PIER = 3,
    PAL_BOARD = 4,
    PAL_WATER = 5,
    PAL_REED = 6,
    PAL_BOBBER = 7,
    PAL_BANNER = 8,
    PAL_WIN = 9,
    PAL_ALERT = 10,
    PAL_LINE = 11
};

struct Art {
    gs::Mipped fish;
    gs::Mipped angler;
    gs::Mipped rod;
    gs::Mipped bobber;
    gs::Mipped pier;
    gs::Mipped board;
    gs::Mipped reed;
    gs::Mipped gull;
    gs::Mipped splash;
    gs::Mipped bead;
    gs::Mipped title;
    gs::Mipped wordMark;
    gs::Mipped finished;
    int font[96] = {};
    int waterTile = 1;
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace fishmark
