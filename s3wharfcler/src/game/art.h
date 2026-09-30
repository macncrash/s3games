// Wharf pictures. Drawn into sprite ROM at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace wcler {

enum Pal {
    PAL_TEXT = 0,
    PAL_GOLD = 1,
    PAL_ALERT = 2,
    PAL_GOOD = 3,
    PAL_WOOD = 4,
    PAL_CREW = 5,
    PAL_WATER = 6,
    PAL_CRATE = 7,
    PAL_ROPE = 8,
    PAL_BARREL = 9,
    PAL_TIMBER = 10,
    PAL_FX = 11,
    PAL_NIGHT = 12
};

struct Art {
    gs::Mipped crew[2];
    gs::Mipped hook;
    gs::Mipped crate;
    gs::Mipped net;
    gs::Mipped barrel;
    gs::Mipped coil;
    gs::Mipped timber;
    gs::Mipped buoy;
    gs::Mipped shed;
    gs::Mipped lamp;
    gs::Mipped piling;
    gs::Mipped gull;
    gs::Mipped splash;
    gs::Mipped shadow;
    gs::Mipped mark;
    gs::Mipped glyph[96];
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace wcler
