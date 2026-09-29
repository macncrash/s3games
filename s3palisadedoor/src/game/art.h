// Palisade pictures. Drawn into the S3-16 at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace palisade {

enum Pal {
    PAL_HUD = 0,
    PAL_WOOD = 1,
    PAL_PLAYER = 2,
    PAL_RAIDER = 3,
    PAL_IRON = 4,
    PAL_FX = 5,
    PAL_MOON = 6
};

struct Art {
    gs::Mipped stake;
    gs::Mipped gate;
    gs::Mipped guard;
    gs::Mipped guardThrust;
    gs::Mipped raider;
    gs::Mipped axe;
    gs::Mipped moon;
    gs::Mipped banner;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace palisade
