// Pictures drawn at boot. Nothing is loaded from a file.
#pragma once

#include "console/gfx.h"
#include "console/vdp.h"

namespace paradechime {

enum Pal {
    PAL_HUD = 0,
    PAL_MAJOR = 1,
    PAL_WAGON = 2,
    PAL_HORSE = 3,
    PAL_BELL = 4,
    PAL_CROWD = 5,
    PAL_FLAG = 6,
    PAL_GOLD = 7,
    PAL_TOWER = 8,
    PAL_CREAM = 9,
    PAL_INK = 10,
    PAL_DRUM = 11,
    PAL_STONE = 12,
    PAL_SKY = 13,
    PAL_CONF = 14,
    PAL_SHADE = 15
};

struct Art {
    int font[96] = {};
    gs::Mipped major;
    gs::Mipped wagon;
    gs::Mipped horse;
    gs::Mipped drum;
    gs::Mipped tower;
    gs::Mipped bell;
    gs::Mipped person;
    gs::Mipped pennant;
    gs::Mipped confetti;
    gs::Mipped shadow;
    gs::Mipped stripe;
    gs::Image title;
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace paradechime
