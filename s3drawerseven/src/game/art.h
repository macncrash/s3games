// S3 DRAWER SEVEN pictures. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace drawerseven {

enum Pal {
    PAL_HUD = 0,
    PAL_GOLD = 1,
    PAL_DIM = 2,
    PAL_BAD = 3,
    PAL_INK = 4,
    PAL_WOOD = 5,
    PAL_COIN = 6,
    PAL_NICK = 7,
    PAL_JUNK = 8,
    PAL_LAMP = 9
};

struct Art {
    gs::Image desk;
    gs::Image drawer;
    gs::Image penny;
    gs::Image nickel;
    gs::Image button;
    gs::Image caret;
    gs::Image lamp;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace drawerseven
