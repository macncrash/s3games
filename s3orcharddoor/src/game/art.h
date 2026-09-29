// S3 ORCHARD DOOR sprites. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"

namespace orcharddoor {

enum Pal {
    PAL_HUD = 0,
    PAL_GRASS = 1,
    PAL_WOOD = 2,
    PAL_LEAF = 3,
    PAL_MAN = 4,
    PAL_IRON = 5,
    PAL_APPLE = 6,
    PAL_ALERT = 7,
    PAL_DUST = 8,
    PAL_BIRD = 9
};

struct Art {
    gs::Mipped tree;
    gs::Mipped door;
    gs::Mipped post;
    gs::Mipped bar;
    gs::Mipped man[2];
    gs::Mipped apple;
    gs::Mipped leaf;
    gs::Mipped hand;
    gs::Mipped mote;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace orcharddoor
