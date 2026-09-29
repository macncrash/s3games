// S3 CAUSEWAY PURSUIT sprites. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace cwpurs {

enum Pal {
    PAL_HUD = 0,
    PAL_YOU = 1,
    PAL_PILE = 2,
    PAL_GRADE = 3,
    PAL_PUMP = 4,
    PAL_LORRY = 5,
    PAL_STONE = 6,
    PAL_LAMP = 7,
    PAL_FX = 8,
    PAL_BOAT = 9,
    PAL_ALERT = 10,
    PAL_GOOD = 11,
    PAL_ROAD = 12,
    PAL_TITLE = 13,
    PAL_MIST = 14,
    PAL_DEAD = 15
};

struct Art {
    gs::Mipped you;
    gs::Mipped pile;
    gs::Mipped grade;
    gs::Mipped pump;
    gs::Mipped lorry;
    gs::Mipped post;
    gs::Mipped lamp;
    gs::Mipped boat;
    gs::Mipped splash;
    gs::Mipped glyph[96];
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace cwpurs
