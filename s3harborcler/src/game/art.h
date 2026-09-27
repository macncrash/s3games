// S3 HARBOR CLER sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace hcler {

enum Pal {
    PAL_TEXT = 0,
    PAL_GOLD = 1,
    PAL_ALERT = 2,
    PAL_GOOD = 3,
    PAL_SHED = 4,
    PAL_CLERK = 5,
    PAL_NET = 6,
    PAL_WOOD = 7,
    PAL_STEEL = 8,
    PAL_FX = 9,
    PAL_NIGHT = 10,
    PAL_BOAT = 11,
    PAL_QUAY = 12,
    PAL_WATER = 13
};

struct Art {
    gs::Mipped clerk[2];
    gs::Mipped barrow;
    gs::Mipped crate;
    gs::Mipped barrel;
    gs::Mipped net;
    gs::Mipped coil;
    gs::Mipped boat;
    gs::Mipped cargo;
    gs::Mipped shed;
    gs::Mipped clock;
    gs::Mipped lamp;
    gs::Mipped crane;
    gs::Mipped piling;
    gs::Mipped buoy;
    gs::Mipped gull;
    gs::Mipped foam;
    gs::Mipped dust;
    gs::Mipped shadow;
    gs::Mipped mark;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace hcler
