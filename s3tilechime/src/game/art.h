// S3 TILECHIME — clock-floor art drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace tilechime {

enum Pal {
    PAL_WALL = 0,
    PAL_CHIME = 1,
    PAL_PLAIN = 2,
    PAL_BELL = 3,
    PAL_HAND = 4,
    PAL_INK = 5,
    PAL_TITLE = 6,
    PAL_WIN = 7,
    PAL_BAD = 8,
    PAL_HINT = 9
};

struct Art {
    gs::Image wall;
    gs::Image tile;
    gs::Image bell;
    gs::Image pip;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace tilechime
