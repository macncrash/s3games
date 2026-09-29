// S3 MUSHPLAT sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace mush {

enum Pal {
    PAL_HUD = 0,
    PAL_SNOW = 1,
    PAL_WOOD = 2,
    PAL_TEAM = 3,
    PAL_PINE = 4,
    PAL_MARK = 5,
    PAL_GOLD = 6
};

struct Art {
    gs::Mipped sled;
    gs::Mipped musher;
    gs::Mipped dog[4];
    gs::Mipped deck;
    gs::Mipped level;
    gs::Mipped post;
    gs::Mipped pine;
    gs::Mipped drift;
    gs::Mipped endWord;
    gs::Mipped levelWord;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace mush
