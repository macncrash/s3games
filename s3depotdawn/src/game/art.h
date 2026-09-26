// S3 DEPOT DAWN sprites. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"

namespace depotdawn {

enum Pal {
    PAL_HUD = 0,
    PAL_SHED = 1,
    PAL_DOCK = 2,
    PAL_MAN = 3,
    PAL_FIRE = 4,
    PAL_EMBER = 5,
    PAL_IRON = 6,
    PAL_LOCO = 7,
    PAL_SMOKE = 8,
    PAL_RAIN = 9,
    PAL_MOON = 10,
    PAL_SUN = 11,
    PAL_YARD = 12,
    PAL_GOLD = 13,
    PAL_ALERT = 14,
    PAL_PIP = 15
};

struct Art {
    gs::Mipped man[2];
    gs::Mipped pot;
    gs::Mipped flame[2];
    gs::Mipped hood;
    gs::Mipped loco[2];
    gs::Mipped wagon;
    gs::Mipped buffer;
    gs::Mipped lamp;
    gs::Mipped rain;
    gs::Mipped smoke[2];
    gs::Mipped spark;
    gs::Mipped shade;
    gs::Mipped moon;
    gs::Mipped sun;
    gs::Mipped star;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace depotdawn
