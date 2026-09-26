// Pictures drawn at boot. Nothing is loaded from a file.
#pragma once

#include "console/gfx.h"
#include "console/vdp.h"

namespace wicketchime {

enum Pal {
    PAL_INK = 0,
    PAL_GOLD = 1,
    PAL_ALERT = 2,
    PAL_GREEN = 3,
    PAL_BALL = 4,
    PAL_BAT = 5,
    PAL_BOWL = 6,
    PAL_WOOD = 7,
    PAL_PITCH = 8,
    PAL_TOWER = 9,
    PAL_FACE = 10,
    PAL_HAND = 11,
    PAL_TREE = 12,
    PAL_SKY = 13,
    PAL_CROWD = 14,
    PAL_SHADE = 15
};

constexpr int kDial = 56;

struct Art {
    int font[96] = {};
    gs::Image hand[3][60];
    gs::Image face, ring, cap;
    gs::Image ball[2];
    gs::Image bail;
    gs::Image bell;
    gs::Image shadow;
    gs::Image blot;
    gs::Image title;
    gs::Mipped bowler[3];
    gs::Mipped batsman[3];
    gs::Mipped keeper;
    gs::Mipped stumps;
    gs::Mipped pitch;
    gs::Mipped tower;
    gs::Mipped screen;
    gs::Mipped tree;
    gs::Mipped house;
    gs::Mipped crowd;
    gs::Mipped rope;
    gs::Mipped cloud;
    gs::Mipped sun;
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace wicketchime
