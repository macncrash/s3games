// S3 YARD BANN sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace yard {

enum Pal {
    PAL_HUD = 0,
    PAL_YARD = 1,
    PAL_SCRAP = 2,
    PAL_BANNER = 3,
    PAL_HAND = 4,
    PAL_DOG = 5,
    PAL_STEEL = 6,
    PAL_FX = 7,
    PAL_ALERT = 8
};

struct Art {
    gs::Mipped stand, runA, runB, pry;
    gs::Mipped dog[3];
    gs::Mipped banner[2];
    gs::Mipped magnet, cable, post, beam, jaw;
    gs::Mipped car[2], shack, hoist, drum, tire, lamp;
    gs::Mipped plate, chevron, mat;
    gs::Mipped spark, shadow, sun;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace yard
