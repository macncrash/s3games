// S3 LANTERN sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace lantern {

enum Pal {
    PAL_HUD = 0,
    PAL_DARK = 1,
    PAL_L0 = 2,
    PAL_L1 = 3,
    PAL_L2 = 4,
    PAL_L3 = 5,
    PAL_L4 = 6,
    PAL_L5 = 7,
    PAL_SCENE = 8,
    PAL_YARD = 9,
    PAL_NO = 10,
    PAL_GOLD = 11,
    PAL_ROSE = 12,
    PAL_JADE = 13,
    PAL_DIM = 14
};

struct Art {
    gs::Mipped lamp[6];
    gs::Mipped glow;
    gs::Mipped flame[2];
    gs::Mipped wick;
    gs::Mipped moon;
    gs::Mipped moth[2];
    gs::Mipped star;
    gs::Mipped cord;
    gs::Mipped beam;
    gs::Mipped post;
    gs::Mipped fly;
    gs::Mipped shade;
    gs::Mipped pane;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace lantern
