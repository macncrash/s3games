// Pictures drawn at boot. Nothing is loaded from a file.
#pragma once

#include "console/gfx.h"
#include "console/vdp.h"

namespace hoopchime {

enum Pal {
    PAL_COURT = 0,
    PAL_BOARD = 1,
    PAL_IRON = 2,
    PAL_BALL = 3,
    PAL_YOU = 4,
    PAL_TOWER = 5,
    PAL_INK = 6,
    PAL_GOLD = 7,
    PAL_ALERT = 8,
    PAL_GREEN = 9,
    PAL_GLASS = 10,
    PAL_METER = 11,
    PAL_FACE = 12,
    PAL_HAND = 13,
    PAL_BELL = 14,
    PAL_WIN = 15
};

constexpr int kDial = 48;

struct Art {
    int font[96] = {};
    gs::Mipped player;
    gs::Mipped ball[2];
    gs::Image rim;
    gs::Image net[2];
    gs::Image board;
    gs::Image pole;
    gs::Image arm;
    gs::Image glass;
    gs::Image cross;
    gs::Image pip;
    gs::Image blot;
    gs::Image shadow;
    gs::Image school;
    gs::Image moon;
    gs::Image face;
    gs::Image ring;
    gs::Image cap;
    gs::Image hand[3][60];
    gs::Image bell;
    gs::Image clapper;
    gs::Image hoopW;
    gs::Image chimeW;
    gs::Image twelveW;
    gs::Image earlyW;
    gs::Image lateW;
    gs::Image shortW;
    gs::Image hotW;
    gs::Image wideW;
    gs::Image ironW;
    gs::Image missW;
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace hoopchime
