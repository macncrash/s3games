// S3 MUSHMARK sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace mushmark {

enum Pal {
    PAL_HUD = 0,
    PAL_SNOW = 1,
    PAL_TEAM = 2,
    PAL_PINE = 3,
    PAL_MARK = 4,
    PAL_GOLD = 5,
    PAL_RIVAL = 6
};

struct Art {
    gs::Mipped sled;
    gs::Mipped musher;
    gs::Mipped dog[3];
    gs::Mipped paint;
    gs::Mipped stake;
    gs::Mipped flag;
    gs::Mipped pine;
    gs::Mipped drift;
    gs::Mipped wordMark;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace mushmark
