// S3 RIDGE DAWN sprites. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"

namespace rdawn {

enum Pal {
    PAL_HUD = 0,
    PAL_GOLD = 1,
    PAL_ALERT = 2,
    PAL_GOOD = 3,
    PAL_YOU = 4,
    PAL_FIRE = 5,
    PAL_EMBER = 6,
    PAL_IRON = 7,
    PAL_SMOKE = 8,
    PAL_WIND = 9,
    PAL_NIGHT = 10,
    PAL_SUN = 11,
    PAL_ROAD = 12,
    PAL_STONE = 13,
    PAL_MOUNT = 14
};

struct Art {
    gs::Mipped warden[2];
    gs::Mipped post;
    gs::Mipped flame[2];
    gs::Mipped glow;
    gs::Mipped smoke[2];
    gs::Mipped gust;
    gs::Mipped spark;
    gs::Mipped shadow;
    gs::Mipped cairn;
    gs::Mipped stake;
    gs::Mipped moon;
    gs::Mipped sun;
    gs::Mipped star;
    gs::Mipped peak[3];
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace rdawn
