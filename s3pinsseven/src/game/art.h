// Pictures drawn at boot. Nothing is loaded from a file.
#pragma once

#include "console/gfx.h"
#include "console/vdp.h"

namespace pinsseven {

enum Pal {
    PAL_HUD = 0,
    PAL_GOLD = 1,
    PAL_AMBER = 2,
    PAL_GREEN = 3,
    PAL_PIN = 4,
    PAL_BALL = 5,
    PAL_HOUSE = 6,
    PAL_YOU = 7,
    PAL_THEM = 8,
    PAL_LAMP = 9,
    PAL_DIM = 10,
    PAL_PIT = 11,
    PAL_LANE = 12,
    PAL_POST = 13
};

struct Art {
    gs::Mipped glyph[96];
    int font[96] = {};
    gs::Mipped pin, pinFlat;
    gs::Mipped ball[3];
    gs::Mipped bowler[3];
    gs::Mipped arrow, dot, foul, shadow, lamp, pit, post;
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace pinsseven
