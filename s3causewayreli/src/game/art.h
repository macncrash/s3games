// S3 CAUSEWAY RELIEF sprites. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace causeway {

enum Pal {
    PAL_HUD = 0,
    PAL_YOU = 1,
    PAL_FOE = 2,
    PAL_TRUCK = 3,
    PAL_BOAT = 4,
    PAL_BELL = 5,
    PAL_FX = 6,
    PAL_OK = 7,
    PAL_ALERT = 8,
    PAL_STONE = 9
};

struct Art {
    gs::Mipped gunner[2];
    gs::Mipped runner[2];
    gs::Mipped truck;
    gs::Mipped boat;
    gs::Mipped bell;
    gs::Mipped post;
    gs::Mipped flash;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace causeway
