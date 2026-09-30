// Slip pictures. Drawn into sprite ROM at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace slip {

enum Pal {
    PAL_TEXT = 0,
    PAL_BUS = 1,
    PAL_QUAY = 2,
    PAL_WATER = 3,
    PAL_SLIP = 4,
    PAL_SKY = 5,
    PAL_ALERT = 6,
    PAL_GOOD = 7,
    PAL_HUD = 8
};

struct Art {
    gs::Mipped bus;
    gs::Mipped wheel;
    gs::Mipped quay;
    gs::Mipped piling;
    gs::Mipped fender;
    gs::Mipped lamp;
    gs::Mipped wave;
    gs::Mipped gull;
    gs::Mipped stop;
    gs::Mipped glyph[96];
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace slip
