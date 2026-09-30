// S3 GRANARY POUC pictures. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace granarypouc {

enum Pal {
    PAL_HUD = 0,
    PAL_WOOD = 1,
    PAL_GRAIN = 2,
    PAL_POUCH = 3,
    PAL_HAND = 4,
    PAL_IRON = 5,
    PAL_DOOR = 6,
    PAL_ALERT = 7,
    PAL_LOFT = 8,
    PAL_DUST = 9
};

struct Art {
    gs::Mipped stand, runA, runB, duck, leap;
    gs::Mipped pouch[2];
    gs::Mipped sack, silo, chute, blade, slab, hatch, bin;
    gs::Mipped lamp, flame[2];
    gs::Mipped hole, grain, lip, shadow;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace granarypouc
