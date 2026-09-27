// S3 SPAN LADD pictures. Drawn into sprite ROM at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace spanladd {

enum Pal {
    PAL_HUD = 0,
    PAL_HERO = 1,
    PAL_WOOD = 2,
    PAL_STONE = 3,
    PAL_IRON = 4,
    PAL_WATER = 5,
    PAL_FLAG = 6,
    PAL_SKY = 7
};

struct Art {
    gs::Mipped hero[2];
    gs::Mipped climb;
    gs::Mipped plank;
    gs::Mipped pier;
    gs::Mipped rung;
    gs::Mipped rail;
    gs::Mipped flag;
    gs::Mipped gull;
    gs::Image wordSpan;
    gs::Image wordFar;
    gs::Image wordGo;
    gs::Image wordDone;
    gs::Image wordHint;
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace spanladd
