// S3 LANTERN GOLD sprites. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace lanterngold {

enum Pal {
    PAL_HUD = 0,
    PAL_DARK = 1,
    PAL_CREAM = 2,
    PAL_GOLD = 3,
    PAL_SCENE = 4,
    PAL_ROSE = 5,
    PAL_JADE = 6,
    PAL_DIM = 7,
    PAL_INK = 8
};

struct Art {
    gs::Mipped lamp;
    gs::Mipped flame[2];
    gs::Mipped wick;
    gs::Mipped moon;
    gs::Mipped cord;
    gs::Mipped beam;
    gs::Mipped post;
    gs::Mipped moth[2];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace lanterngold
