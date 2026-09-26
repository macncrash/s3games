// Pictures drawn at boot. Nothing is loaded from a file.
#pragma once

#include "console/gfx.h"
#include "console/vdp.h"

namespace pinschime {

enum Pal {
    PAL_INK = 0,
    PAL_GOLD = 1,
    PAL_CREAM = 2,
    PAL_BELL = 3,
    PAL_BALL = 4,
    PAL_BOWLER = 5,
    PAL_AMBER = 6,
    PAL_GREEN = 7,
    PAL_RED = 8,
    PAL_FACE = 9,
    PAL_HAND = 10,
    PAL_TOWER = 11,
    PAL_LANE = 12
};

struct Art {
    gs::Mipped glyph[96];
    int font[96] = {};
    gs::Image hand[3][60];
    gs::Image face, cap, ring;
    gs::Mipped pin, pinFlat, bellPin, bellFlat;
    gs::Mipped ball[4];
    gs::Mipped bowler[4];
    gs::Mipped arrow, dot, foul, shadow, lamp, tower, bell;
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace pinschime
