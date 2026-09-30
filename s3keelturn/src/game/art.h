// S3 KEEL TURN sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace keel {

enum Pal {
    PAL_HUD = 0,
    PAL_BOAT = 1,
    PAL_GHOST = 2,
    PAL_MARK = 3,
    PAL_CREW = 4,
    PAL_FIELD = 12
};

struct Art {
    gs::Mipped boat[5];
    gs::Mipped ghost;
    gs::Mipped buoy;
    gs::Mipped wake;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace keel
