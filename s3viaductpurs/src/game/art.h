// S3 VIADUCT PURSUIT pictures. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace viaductpurs {

enum Pal {
    PAL_HUD = 0,
    PAL_YOU = 1,
    PAL_RAIL = 2,
    PAL_CRANE = 3,
    PAL_BUS = 4,
    PAL_BOLT = 5,
    PAL_STONE = 6,
    PAL_OK = 7,
    PAL_ALERT = 8
};

struct Art {
    gs::Mipped runner[2];
    gs::Mipped rail[2];
    gs::Mipped crane[2];
    gs::Mipped bus[2];
    gs::Mipped bolt;
    gs::Mipped puff;
    gs::Mipped pier;
    gs::Mipped lamp;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace viaductpurs
