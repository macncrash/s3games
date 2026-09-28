// S3 BUNKER CLER sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace bcler {

enum Pal {
    PAL_TEXT = 0,
    PAL_GOLD = 1,
    PAL_ALERT = 2,
    PAL_GOOD = 3,
    PAL_CONCRETE = 4,
    PAL_SENTRY = 5,
    PAL_EARTH = 6,
    PAL_STEEL = 7,
    PAL_WIRE = 8,
    PAL_SAND = 9,
    PAL_WOOD = 10,
    PAL_FX = 11,
    PAL_NIGHT = 12
};

struct Art {
    gs::Mipped sentry[2];
    gs::Mipped shovel;
    gs::Mipped rubble;
    gs::Mipped coil;
    gs::Mipped shell;
    gs::Mipped bags;
    gs::Mipped timber;
    gs::Mipped drum;
    gs::Mipped bunker;
    gs::Mipped slit;
    gs::Mipped lamp;
    gs::Mipped beam;
    gs::Mipped stake;
    gs::Mipped dust;
    gs::Mipped shadow;
    gs::Mipped mark;
    gs::Mipped glyph[96];
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace bcler
