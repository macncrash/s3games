// Palisade clearer sprites. Drawn into the VDP at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace palcler {

enum Pal {
    PAL_HUD = 0,
    PAL_TIMBER = 1,
    PAL_WARD = 2,
    PAL_IRON = 3,
    PAL_BRUSH = 4,
    PAL_DUSK = 5,
    PAL_STONE = 6,
    PAL_ALERT = 7
};

struct Art {
    gs::Mipped ward[2];
    gs::Mipped rake;
    gs::Mipped stake, gate, stone, branch, shield, brush, mote, ditch;
    gs::Mipped glyph[96];
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace palcler
