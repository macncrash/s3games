// Pictures drawn at boot. Nothing is loaded from a file.
#pragma once

#include "console/gfx.h"
#include "console/vdp.h"

namespace pinsbell {

enum Pal {
    PAL_HUD = 0,
    PAL_AMBER = 1,
    PAL_PIN = 2,
    PAL_BALL = 3,
    PAL_BOWLER = 4,
    PAL_BELL = 5,
    PAL_GREEN = 6,
    PAL_LAMP = 7,
    PAL_DEAD = 8,
    PAL_GOLD = 9,
    PAL_CURTAIN = 10,
    PAL_HEAD = 11,
    PAL_LANE = 12,
    PAL_ALERT = 13,
    PAL_POST = 14
};

struct Art {
    gs::Mipped glyph[96];
    int font[96] = {};
    gs::Mipped pin, pinFlat;
    gs::Mipped ball[3];
    gs::Mipped bowler[4];
    gs::Mipped bell, clapper, yoke;
    gs::Mipped arrow, dot, foul, shadow, curtain, lamp, post;
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace pinsbell
