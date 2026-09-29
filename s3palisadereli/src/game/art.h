// Palisade sprites, drawn into the VDP at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace palisade {

enum Pal {
    PAL_HUD = 0,
    PAL_WOOD = 1,
    PAL_YOU = 2,
    PAL_FOE = 3,
    PAL_SHIELD = 4,
    PAL_FX = 5,
    PAL_BELL = 6,
    PAL_ALERT = 7,
    PAL_OK = 8,
    PAL_FIELD = 9
};

struct Art {
    gs::Mipped stake, gate, hill, skyband;
    gs::Mipped spear, you[2];
    gs::Mipped climber[2], runner[2], shield[2];
    gs::Mipped bell, rope, spark;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace palisade
