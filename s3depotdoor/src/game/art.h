// S3 DEPOT DOOR pictures. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace depotdoor {

enum Pal {
    PAL_TEXT = 0,
    PAL_GOLD = 1,
    PAL_ALERT = 2,
    PAL_GOOD = 3,
    PAL_BRICK = 4,
    PAL_STEEL = 5,
    PAL_COAT = 6,
    PAL_DOCKER = 7,
    PAL_FREIGHT = 8,
    PAL_LAMP = 9,
    PAL_IRON = 10,
    PAL_NIGHT = 11,
    PAL_ROAD = 12,
    PAL_CAR = 13,
    PAL_BAY = 14
};

struct Art {
    gs::Mipped clerk[3];
    gs::Mipped docker[2];
    gs::Mipped truck;
    gs::Mipped slat;
    gs::Mipped pier;
    gs::Mipped beam;
    gs::Mipped sprocket;
    gs::Mipped chain;
    gs::Mipped bolt;
    gs::Mipped sack;
    gs::Mipped crate;
    gs::Mipped drum;
    gs::Mipped boxcar;
    gs::Mipped lamp[2];
    gs::Mipped bay;
    gs::Mipped sign;
    gs::Mipped clock;
    gs::Mipped spark;
    gs::Mipped chip;
    gs::Mipped shadow;
    gs::Mipped moon;
    gs::Mipped star;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace depotdoor
