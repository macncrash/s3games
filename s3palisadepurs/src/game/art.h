// S3 PALISADE PURSUIT sprites. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace palisadepurs {

enum Pal {
    PAL_HUD = 0,
    PAL_STAKE = 1,
    PAL_YOU = 2,
    PAL_RAM = 3,
    PAL_CART = 4,
    PAL_BOLT = 5,
    PAL_FX = 6,
    PAL_FIELD = 7,
    PAL_WRECK = 8
};

struct Art {
    gs::Mipped stake;
    gs::Mipped you;
    gs::Mipped ram;
    gs::Mipped cart;
    gs::Mipped bolt;
    gs::Mipped shot;
    gs::Mipped spark;
    gs::Mipped lamp;
    gs::Mipped glyph[96];
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace palisadepurs
