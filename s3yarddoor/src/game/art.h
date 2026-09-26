// S3 YARD DOOR pictures. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace yarddoor {

enum Pal {
    PAL_TEXT = 0,
    PAL_GOLD = 1,
    PAL_ALERT = 2,
    PAL_GOOD = 3,
    PAL_WOOD = 4,
    PAL_COAT = 5,
    PAL_IRON = 6,
    PAL_BREAK = 7,
    PAL_PROP = 8,
    PAL_FIRE = 9,
    PAL_SKY = 10,
    PAL_DUST = 11,
    PAL_ROAD = 12
};

struct Art {
    gs::Mipped leaf;
    gs::Mipped post;
    gs::Mipped beam;
    gs::Mipped crib;
    gs::Mipped hand[2];
    gs::Mipped breaker[2];
    gs::Mipped log;
    gs::Mipped bar;
    gs::Mipped chock;
    gs::Mipped lantern;
    gs::Mipped flame;
    gs::Mipped hoist;
    gs::Mipped pennant[2];
    gs::Mipped keg;
    gs::Mipped coil;
    gs::Mipped horse;
    gs::Mipped barrow;
    gs::Mipped pip;
    gs::Mipped stripe;
    gs::Mipped spark;
    gs::Mipped shadow;
    gs::Mipped rail;
    gs::Mipped cloud;
    gs::Mipped sun;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace yarddoor
