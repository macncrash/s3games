// S3 KARTPLAT sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace kart {

enum Pal {
    PAL_HUD = 0,
    PAL_TAR = 1,
    PAL_PIT = 2,
    PAL_KART = 3,
    PAL_TREE = 4,
    PAL_END = 5,
    PAL_GOLD = 6
};

struct Art {
    gs::Mipped body;
    gs::Mipped driver;
    gs::Mipped wheel[4];
    gs::Mipped deck;
    gs::Mipped level;
    gs::Mipped pylon;
    gs::Mipped stand;
    gs::Mipped endWord;
    gs::Mipped levelWord;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace kart
