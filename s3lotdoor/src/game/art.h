// S3 LOT DOOR sprites. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"

namespace lotdoor {

enum Pal {
    PAL_HUD = 0,
    PAL_GATE = 1,
    PAL_CART = 2,
    PAL_SEDAN = 3,
    PAL_TRUCK = 4,
    PAL_FLARE = 5,
    PAL_LAMP = 6,
    PAL_GOLD = 7,
    PAL_ALERT = 8,
    PAL_LOT = 12
};

struct Art {
    gs::Mipped gate;
    gs::Mipped cart;
    gs::Mipped sedan;
    gs::Mipped truck;
    gs::Mipped flare;
    gs::Mipped spark;
    gs::Mipped lamp;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace lotdoor
