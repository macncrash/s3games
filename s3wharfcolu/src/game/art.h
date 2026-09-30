// S3 WHARF COLUMN sprites. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace whc {

enum Pal {
    PAL_TEXT = 0,
    PAL_SALT = 1,
    PAL_ALERT = 2,
    PAL_GOOD = 3,
    PAL_LORRY = 4,
    PAL_CART = 5,
    PAL_SHED = 6,
    PAL_LAMP = 7,
    PAL_CRATE = 8,
    PAL_DUST = 9,
    PAL_CHAIN = 10,
    PAL_ROAD = 12
};

struct Art {
    gs::Mipped lorry, cart, shed, lamp, hook;
    gs::Mipped chain, stripe, crate, buoy, dust;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace whc
