// S3 GATE DAWN sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"

namespace gatedawn {

enum Pal {
    PAL_HUD = 0,
    PAL_STONE = 1,
    PAL_ROAD = 2,
    PAL_MAN = 3,
    PAL_FIRE = 4,
    PAL_EMBER = 5,
    PAL_IRON = 6,
    PAL_SNEAK = 7,
    PAL_WIND = 8,
    PAL_MOON = 9,
    PAL_SUN = 10,
    PAL_SMOKE = 11,
    PAL_ALERT = 12,
    PAL_GOLD = 13,
    PAL_PIP = 14
};

struct Art {
    gs::Mipped man[2];
    gs::Mipped sneak[2];
    gs::Mipped flame[2];
    gs::Mipped basket;
    gs::Mipped post;
    gs::Mipped gust;
    gs::Mipped moon;
    gs::Mipped sun;
    gs::Mipped star;
    gs::Mipped smoke[2];
    gs::Mipped pip;
    gs::Mipped shade;
    gs::Mipped spark;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace gatedawn
