// Pictures for the lake. Drawn at boot. No asset files.
#pragma once

#include "console/gfx.h"
#include "console/vdp.h"

namespace fishseven {

enum Pal {
    PAL_FISH = 0,
    PAL_RIVAL = 1,
    PAL_BOAT = 2,
    PAL_LURE = 3,
    PAL_REED = 4,
    PAL_SUN = 5,
    PAL_HUD = 6,
    PAL_INK = 7
};

struct Art {
    gs::Image fish;
    gs::Image boat;
    gs::Image angler;
    gs::Image lure;
    gs::Image reed;
    gs::Image sun;
    gs::Image glyph[96];
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace fishseven
