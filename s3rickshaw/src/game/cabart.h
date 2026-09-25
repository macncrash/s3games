// S3 RICKSHAW pictures. Drawn into sprite ROM at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace rickshaw {

enum Pal {
    PAL_HUD = 0,
    PAL_ALERT = 1,
    PAL_WIN = 2,
    PAL_BANNER = 3,
    PAL_CAB = 4,
    PAL_CITY = 5,
    PAL_TEMPLE = 6,
    PAL_SIGN = 7,
    PAL_LIFE = 8,
    PAL_STREET = 12
};

struct Art {
    gs::Mipped cab[9];
    gs::Mipped spilled;
    gs::Mipped shop, house, temple, shed;
    gs::Mipped lamp, stall, signL, signR, post;
    gs::Mipped cow, dog, bird[2], flower, sun, dust;
    gs::Mipped title, sub, paid, tipped;
    float pivX = 0, pivY = 0;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace rickshaw
