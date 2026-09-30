// S3 FLUTEMARK sprites. Drawn into VDP RAM at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace flutemark {

enum Pal {
    PAL_HUD = 0,
    PAL_GOLD = 1,
    PAL_RED = 2,
    PAL_GREEN = 3,
    PAL_WOOD = 4,
    PAL_PLAYER = 5,
    PAL_NIGHT = 6,
    PAL_LAMP = 7,
    PAL_NOTE = 8
};

struct Art {
    gs::Mipped player;
    gs::Mipped flute;
    gs::Mipped stand;
    gs::Mipped staff;
    gs::Mipped note;
    gs::Mipped noteOn;
    gs::Mipped breath;
    gs::Mipped window;
    gs::Mipped lamp;
    gs::Mipped card;
    gs::Mipped stamp;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace flutemark
