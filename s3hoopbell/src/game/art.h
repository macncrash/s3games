// Pictures drawn at boot. Nothing is loaded from a file.
#pragma once

#include "console/gfx.h"
#include "console/vdp.h"

namespace hoopbell {

enum Pal {
    PAL_COURT = 0,
    PAL_BOARD = 1,
    PAL_IRON = 2,
    PAL_BALL = 3,
    PAL_YOU = 4,
    PAL_BELL = 5,
    PAL_INK = 6,
    PAL_GOLD = 7,
    PAL_ALERT = 8,
    PAL_GREEN = 9,
    PAL_GLASS = 10,
    PAL_METER = 11,
    PAL_SUN = 12,
    PAL_ASH = 13
};

struct Art {
    int font[96] = {};
    gs::Mipped player;
    gs::Mipped ball[2];
    gs::Mipped bell;
    gs::Image clapper;
    gs::Image rim;
    gs::Image net[2];
    gs::Image board;
    gs::Image pole;
    gs::Image arm;
    gs::Image glass;
    gs::Image yard;
    gs::Image sun;
    gs::Image cross;
    gs::Image pip;
    gs::Image blot;
    gs::Image shadow;
    gs::Image hoop;
    gs::Image bellWord;
    gs::Image rung;
    gs::Image deadW;
    gs::Image leaveW;
    gs::Image shortW;
    gs::Image hotW;
    gs::Image ironW;
    gs::Image missW;
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace hoopbell
