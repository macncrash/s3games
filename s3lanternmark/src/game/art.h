// S3 LANTERNMARK sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace lanternmark {

enum Pal {
    PAL_HUD = 0,
    PAL_DARK = 1,
    PAL_L0 = 2,
    PAL_L1 = 3,
    PAL_L2 = 4,
    PAL_L3 = 5,
    PAL_SCENE = 6,
    PAL_GOLD = 7,
    PAL_ROSE = 8,
    PAL_JADE = 9,
    PAL_DIM = 10,
    PAL_YARD = 11
};

struct Art {
    gs::Mipped lamp;
    gs::Mipped ring;
    gs::Mipped flame[2];
    gs::Mipped glow;
    gs::Mipped wick;
    gs::Mipped cord;
    gs::Mipped beam;
    gs::Mipped post;
    gs::Mipped moon;
    gs::Mipped star;
    gs::Mipped moth[2];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace lanternmark
