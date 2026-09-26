// Pictures drawn at boot. Nothing is loaded from a file.
#pragma once

#include "console/gfx.h"
#include "console/vdp.h"

namespace hoopseven {

enum Pal {
    PAL_INK = 0,
    PAL_YOU = 1,
    PAL_LANE = 2,
    PAL_BALL = 3,
    PAL_RIM = 4,
    PAL_BOARD = 5,
    PAL_IRON = 6,
    PAL_NET = 7,
    PAL_GOOD = 8,
    PAL_BAD = 9,
    PAL_COURT = 10,
    PAL_METER = 11,
    PAL_LAMP = 12,
    PAL_SEAT = 13,
    PAL_WORD = 14,
    PAL_SHADOW = 15
};

struct Art {
    int font[96] = {};
    gs::Mipped ball[2];
    gs::Mipped player[2];
    gs::Mipped rim, net[2], board, pole, lamp, moon, fence, seats;
    gs::Image solid, shadow, bracket;
    gs::Image hoop, seven, first, floor7;
    gs::Image swish, bank, count, rimWord, shortWord, longWord, airWord;
    gs::Image num[3];
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace hoopseven
