// S3 CISTERN CLER sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace ccler {

enum Pal {
    PAL_TEXT = 0,
    PAL_STONE = 1,
    PAL_WARD = 2,
    PAL_SILT = 3,
    PAL_MOSS = 4,
    PAL_WATER = 5,
    PAL_WOOD = 6,
    PAL_RIM = 7,
    PAL_GOOD = 8,
    PAL_ALERT = 9,
    PAL_DUSK = 10
};

struct Art {
    gs::Mipped ward[2];
    gs::Mipped rake;
    gs::Mipped silt;
    gs::Mipped moss;
    gs::Mipped brick;
    gs::Mipped flask;
    gs::Mipped reed;
    gs::Mipped pail;
    gs::Mipped plank;
    gs::Mipped pier;
    gs::Mipped clock;
    gs::Mipped ring;
    gs::Mipped shadow;
    gs::Mipped puff;
    gs::Mipped glyph[96];
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace ccler
