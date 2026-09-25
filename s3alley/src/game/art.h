// S3 ALLEY sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace alley {

enum Pal {
    PAL_HUD = 0,
    PAL_COAT = 1,
    PAL_BRICK = 2,
    PAL_WOOD = 3,
    PAL_LAMP = 4,
    PAL_CLOTH = 5,
    PAL_SHADE = 6,
    PAL_RAIN = 7,
    PAL_AMBER = 8,
    PAL_ALERT = 9,
    PAL_WARM = 11,
    PAL_FIELD = 12
};

struct Art {
    gs::Mipped walker[2];
    gs::Mipped crouch;
    gs::Mipped shadow;
    gs::Mipped crate;
    gs::Mipped barrel;
    gs::Mipped sign;
    gs::Mipped laundry;
    gs::Mipped vent;
    gs::Mipped lamp;
    gs::Mipped wall[4];
    gs::Mipped endwall;
    gs::Mipped door;
    gs::Mipped glow;
    gs::Mipped rain;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace alley
