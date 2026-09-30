// S3 WHARF RELIEF sprites. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace wharf {

enum Pal {
    PAL_HUD = 0,
    PAL_YOU = 1,
    PAL_FOE = 2,
    PAL_TUG = 3,
    PAL_SKIFF = 4,
    PAL_BELL = 5,
    PAL_FX = 6,
    PAL_OK = 7,
    PAL_ALERT = 8,
    PAL_WOOD = 9
};

struct Art {
    gs::Mipped sentry[2];
    gs::Mipped boarder[2];
    gs::Mipped skiff;
    gs::Mipped tug;
    gs::Mipped bell;
    gs::Mipped piling;
    gs::Mipped crate;
    gs::Mipped lantern;
    gs::Mipped flash;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace wharf
