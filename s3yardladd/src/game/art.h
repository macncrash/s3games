// S3 YARD LADD sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace yardladd {

enum Pal {
    PAL_HUD = 0,
    PAL_YARD = 1,
    PAL_HAND = 2,
    PAL_IRON = 3,
    PAL_BRASS = 4,
    PAL_WATCH = 5,
    PAL_WOOD = 6,
    PAL_FX = 7,
    PAL_ALERT = 8,
    PAL_OK = 9,
    PAL_FAR = 10,
    PAL_GOLD = 11,
    PAL_DIM = 12
};

struct Art {
    gs::Mipped stand, walkA, walkB, jump, climbA, climbB;
    gs::Mipped ladder, gate, post, motor, bell;
    gs::Mipped lamp, flame[2], banner, drum, sentry[2];
    gs::Mipped moon, dust, shadow, signal;
    gs::Mipped glyph[96];
    int font[96] = {};
    int cope = 1, brick = 1, brickB = 1, crate = 1, crateB = 1;
    int gravel = 1, gravelB = 1, pit = 1, slit = 1, slitLit = 1;
    int star = 1, starB = 1, shed = 1, roof = 1, win = 1, winLit = 1;
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace yardladd
