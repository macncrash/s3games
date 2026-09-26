// S3 BEDS pictures. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace beds {

enum Pal {
    PAL_HUD = 0,
    PAL_GOLD = 1,
    PAL_WARN = 2,
    PAL_GOOD = 3,
    PAL_DIM = 4,
    PAL_WOOD = 5,
    PAL_DRY = 6,
    PAL_WET = 7,
    PAL_PLANT = 8,
    PAL_MAN = 9,
    PAL_SUN = 10,
    PAL_LIGHT = 11,
    PAL_BRICK = 12,
    PAL_YARD = 13,
    PAL_WATER = 14
};

struct Art {
    gs::Mipped logo, sub, tag, watered, late;
    gs::Mipped frame, soil;
    gs::Mipped plant[6][4];  // crop, stage: sprout, mid, full, wilt
    gs::Mipped man[4];
    gs::Mipped shadow, sun[4], cloud, bird[2];
    gs::Mipped light, drop, spark;
    gs::Mipped barBg, barFg;
    gs::Mipped barrel, vine;
    int grass[4] = {};
    int gravel[2] = {};
    int brick[2] = {};
    int pier = 0, cap = 0;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace beds
