// Pictures drawn at boot. Nothing is loaded from a file.
#pragma once

#include "console/gfx.h"
#include "console/vdp.h"

namespace pinsmark {

enum Pal {
    PAL_HUD = 0,
    PAL_PAPER = 1,
    PAL_PIN = 2,
    PAL_BALL = 3,
    PAL_BOWLER = 4,
    PAL_BRASS = 5,
    PAL_OK = 6,
    PAL_BAD = 7,
    PAL_DECK = 8,
    PAL_ARROW = 9,
    PAL_LAMP = 10,
    PAL_LANE = 12
};

struct Art {
    int font[96] = {};
    gs::Mipped pin, pinFlat;
    gs::Mipped ball[3];
    gs::Mipped bowler[3];
    gs::Mipped arrow, dot, spot, foul, shadow, machine, backstop, lamp;
    gs::Image title, finished, leave, open, markOpen;
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace pinsmark
