// S3 HARBOR POUC sprites. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace harborpouc {

enum Pal {
    PAL_HUD = 0,
    PAL_PLAYER = 1,
    PAL_POUCH = 2,
    PAL_WOOD = 3,
    PAL_STEEL = 4,
    PAL_WATER = 5,
    PAL_ALERT = 6,
    PAL_GO = 7,
    PAL_HOUSE = 8,
    PAL_FOAM = 9
};

struct Art {
    gs::Mipped stand, duck;
    gs::Mipped pouch;
    gs::Mipped plank, pontoon, bollard, shed;
    gs::Mipped post, beam, hook, cable;
    gs::Mipped buoy, gull, lamp, wave;
    gs::Mipped shadow;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace harborpouc
