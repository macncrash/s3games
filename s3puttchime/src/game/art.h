// Pictures drawn at boot. Nothing is loaded from a file.
#pragma once

#include "console/gfx.h"
#include "console/vdp.h"

namespace puttchime {

enum Pal {
    PAL_HUD = 0,
    PAL_GOLD = 1,
    PAL_ALERT = 2,
    PAL_GREEN = 3,
    PAL_BALL = 4,
    PAL_FLAG = 5,
    PAL_TOWER = 6,
    PAL_CUP = 7,
    PAL_TREE = 8,
    PAL_AIM = 9,
    PAL_PLAYER = 10,
    PAL_GRASS = 11,
    PAL_SKY = 12,
    PAL_FACE = 13,
    PAL_HAND = 14,
    PAL_BELL = 15
};

struct Art {
    int font[96] = {};
    int grassA = 1, grassB = 1, rough = 1, hedge = 1;
    gs::Image hand[3][60];
    gs::Image face, ring, cap;
    gs::Mipped ball[2];
    gs::Mipped shadow;
    gs::Mipped cup;
    gs::Mipped flag[2];
    gs::Mipped golfer[3];
    gs::Mipped tower;
    gs::Mipped bell;
    gs::Mipped tree;
    gs::Mipped flower;
    gs::Mipped cloud;
    gs::Mipped sun;
    gs::Mipped dot;
    gs::Mipped borrow;
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace puttchime
