// S3 TOWER WELL sprites. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace tww {

enum Pal {
    PAL_HUD = 0,
    PAL_STONE = 1,
    PAL_WELL = 2,
    PAL_WOOD = 3,
    PAL_RAIDER = 4,
    PAL_BRUTE = 5,
    PAL_FX = 6,
    PAL_FIELD = 7,
    PAL_SKY = 8
};

struct Art {
    gs::Mipped tower, well, wellHurt, bucket, raider, brute, bolt, puff, tree, reed, banner, slit;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace tww
