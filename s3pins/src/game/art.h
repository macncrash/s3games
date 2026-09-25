// Pictures drawn at boot. Nothing is loaded from a file.
#pragma once

#include "console/gfx.h"
#include "console/vdp.h"

namespace pins {

enum Pal {
    PAL_HUD = 0,
    PAL_AMBER = 1,
    PAL_PIN = 2,
    PAL_BALL = 3,
    PAL_BOWLER = 4,
    PAL_GOLD = 5,
    PAL_GREEN = 6,
    PAL_LAMP = 7,
    PAL_RETURN = 8,
    PAL_GLOW = 9,
    PAL_CURTAIN = 10,
    PAL_LANE = 12
};

struct Art {
    gs::Mipped glyph[96];
    int font[96] = {};
    gs::Mipped pin, pinFlat;
    gs::Mipped ball[4];
    gs::Mipped bowler[4];
    gs::Mipped arrow, dot, foul, shadow, glow, lamp, ret, curtain;
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace pins
