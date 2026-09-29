// S3 PALISADE LADD sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace palisadeladd {

enum Pal {
    PAL_HUD = 0,
    PAL_SKY = 1,
    PAL_STAKE = 2,
    PAL_HERO = 3,
    PAL_LOG = 4,
    PAL_FIRE = 5,
    PAL_LADDER = 6,
    PAL_BELL = 7,
    PAL_GOLD = 8,
    PAL_ALERT = 9,
    PAL_OK = 10,
    PAL_FAR = 11
};

struct Art {
    gs::Mipped stand, walkA, walkB, jump, climbA, climbB;
    gs::Mipped ladder, log, flame, bell, banner;
    gs::Mipped glyph[96];
    int font[96] = {};
    int plank = 1, plankEnd = 1, stake = 1, tip = 1;
    int star = 1, ridge = 1, ridgeTip = 1;
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace palisadeladd
