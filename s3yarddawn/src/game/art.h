// S3 YARD DAWN sprites. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"

namespace yarddawn {

enum Pal {
    PAL_HUD = 0,
    PAL_YARD = 1,
    PAL_GRAVEL = 2,
    PAL_MAN = 3,
    PAL_FIRE = 4,
    PAL_EMBER = 5,
    PAL_IRON = 6,
    PAL_CANVAS = 7,
    PAL_DRUM = 8,
    PAL_SMOKE = 9,
    PAL_MOON = 10,
    PAL_SUN = 11,
    PAL_GOLD = 12,
    PAL_ALERT = 13,
    PAL_PIP = 14,
    PAL_LAMP = 15
};

struct Art {
    gs::Mipped man[2];
    gs::Mipped pot;
    gs::Mipped flame[2];
    gs::Mipped canvas;
    gs::Mipped drum[2];
    gs::Mipped smoke[2];
    gs::Mipped spark;
    gs::Mipped shade;
    gs::Mipped moon;
    gs::Mipped sun;
    gs::Mipped star;
    gs::Mipped lamp;
    gs::Mipped glyph[96];
    int font[96] = {};
    int bar = 0;
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace yarddawn
