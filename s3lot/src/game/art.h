// S3 LOT pictures. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace lot {

enum Pal {
    PAL_HUD = 0,
    PAL_DIM = 1,
    PAL_ALERT = 2,
    PAL_OK = 3,
    PAL_GOLD = 4,
    PAL_KID = 5,
    PAL_T1 = 6,
    PAL_T2 = 7,
    PAL_T3 = 8,
    PAL_GATE = 9,
    PAL_BALL = 10,
    PAL_JUNK = 11,
    PAL_FX = 12,
    PAL_SKY = 13,
    PAL_FIELD = 14
};

struct Art {
    gs::Mipped kidFront, kidBack, kidSide;
    gs::Mipped targetUp, targetDown;
    gs::Mipped post, bar;
    gs::Mipped ball, puff, shadow, dot;
    gs::Mipped drum, crate, tire, weed, lamp;
    gs::Mipped sun, star;
    gs::Mipped glyph[96];
    int font[96] = {};
    int grass[2] = {};
    int walk = 0;
    int chain = 0;
    int fencePost = 0;
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace lot
