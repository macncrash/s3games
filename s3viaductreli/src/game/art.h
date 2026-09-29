// S3 VIADUCT sprites. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace viaduct {

enum Pal {
    PAL_HUD = 0,
    PAL_YOU = 1,
    PAL_FOE = 2,
    PAL_FUSE = 3,
    PAL_BELL = 4,
    PAL_STONE = 5,
    PAL_OK = 6,
    PAL_ALERT = 7,
    PAL_GORGE = 8
};

struct Art {
    gs::Mipped sentry[2];
    gs::Mipped climber[2];
    gs::Mipped arch;
    gs::Mipped bell;
    gs::Mipped spark;
    gs::Mipped lamp;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace viaduct
