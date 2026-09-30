// S3 FLUTE SEVEN sprites. Drawn into VDP RAM at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace fluteseven {

constexpr int kSeven = 7;

enum Pal {
    PAL_HUD = 0,
    PAL_GOLD = 1,
    PAL_RED = 2,
    PAL_GREEN = 3,
    PAL_WOOD = 4,
    PAL_YOU = 5,
    PAL_THEM = 6,
    PAL_HALL = 7,
    PAL_NOTE = 8
};

struct Art {
    gs::Mipped you;
    gs::Mipped them;
    gs::Mipped flute;
    gs::Mipped staff;
    gs::Mipped note;
    gs::Mipped noteOn;
    gs::Mipped breath;
    gs::Mipped arch;
    gs::Mipped lamp;
    gs::Mipped pip;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace fluteseven
