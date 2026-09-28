// S3 LOT DAWN sprites. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"

namespace lotdawn {

enum Pal {
    PAL_HUD = 0,
    PAL_LOT = 1,
    PAL_CAR = 2,
    PAL_MAN = 3,
    PAL_FIRE = 4,
    PAL_EMBER = 5,
    PAL_IRON = 6,
    PAL_DRUM = 7,
    PAL_LAMP = 8,
    PAL_SMOKE = 9,
    PAL_MOON = 10,
    PAL_SIGN = 11,
    PAL_GOLD = 12,
    PAL_ALERT = 13,
    PAL_PIP = 14,
    PAL_VEST = 15
};

struct Art {
    gs::Mipped man[2];
    gs::Mipped pot;
    gs::Mipped flame[2];
    gs::Mipped car[2];
    gs::Mipped drum;
    gs::Mipped lamp;
    gs::Mipped spark;
    gs::Mipped moon;
    gs::Mipped glyph[96];
    int font[96] = {};
    int asphalt = 0;
    int stall = 0;
    int curb = 0;
    int dark = 0;
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace lotdawn
