// S3 PLOWPLAT sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace plow {

enum Pal {
    PAL_HUD = 0,
    PAL_PLOW = 1,
    PAL_DECK = 2,
    PAL_SNOW = 3,
    PAL_PINE = 4,
    PAL_BARN = 5,
    PAL_MARK = 6,
    PAL_SPRAY = 7
};

struct Art {
    gs::Mipped plow;
    gs::Mipped deck;
    gs::Mipped post;
    gs::Mipped pine;
    gs::Mipped barn;
    gs::Mipped snow;
    gs::Mipped spray;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace plow
