// S3 TRENCH PURS sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace purs {

enum Pal {
    PAL_INK = 0,
    PAL_AMBER = 1,
    PAL_ALERT = 2,
    PAL_GUN = 3,
    PAL_FOE = 4,
    PAL_FX = 5,
    PAL_BAG = 6,
    PAL_DEAD = 7,
    PAL_ROAD = 12
};

struct Art {
    gs::Mipped glyph[96];
    gs::Mipped gun;
    gs::Mipped dead;
    gs::Mipped foe;
    gs::Mipped flash;
    gs::Mipped bag;
    gs::Mipped post;
    gs::Mipped star;
    gs::Mipped flare;
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace purs
