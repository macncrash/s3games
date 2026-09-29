// S3 CAUSEWAY WELL sprites. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace cwell {

enum Pal {
    PAL_HUD = 0,
    PAL_YOU = 1,
    PAL_FOE = 2,
    PAL_HAUL = 3,
    PAL_WELL = 4,
    PAL_FX = 5,
    PAL_OK = 6,
    PAL_ALERT = 7,
    PAL_POST = 8,
    PAL_SKIFF = 9
};

struct Art {
    gs::Mipped keeper[2];
    gs::Mipped raider[2];
    gs::Mipped hauler;
    gs::Mipped well;
    gs::Mipped crack;
    gs::Mipped shot;
    gs::Mipped flash;
    gs::Mipped post;
    gs::Mipped skiff;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace cwell
