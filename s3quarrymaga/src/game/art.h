// S3 QUARRY MAGA pictures. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace qmaga {

enum Pal {
    PAL_HUD = 0,
    PAL_LIME = 1,
    PAL_CRANE = 2,
    PAL_BRASS = 3,
    PAL_CUT = 4,
    PAL_DUST = 5,
    PAL_FX = 6,
    PAL_ALERT = 7,
    PAL_OK = 8,
    PAL_WATCH = 9
};

struct Art {
    gs::Mipped bench, crane, crusher, hardhat, cut[2], dust[2], down, brass, chev, flash, puff;
    gs::Image glyph[96];
    int gw[96] = {};
    int gh = 8;
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace qmaga
