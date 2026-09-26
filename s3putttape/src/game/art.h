// S3 PUTTTAPE pictures. Everything is drawn at boot. There are no asset files.
#pragma once

#include "console/gfx.h"
#include "console/vdp.h"

namespace putttape {

enum Pal {
    PAL_TEXT = 0,
    PAL_GOLD = 1,
    PAL_SILVER = 2,
    PAL_COPPER = 3,
    PAL_TOKEN = 4,
    PAL_GREEN = 5,
    PAL_RED = 6,
    PAL_BALL = 7,
    PAL_WOOD = 8,
    PAL_PAPER = 9,
    PAL_PLAYER = 10,
    PAL_TREE = 11,
    PAL_AIM = 12,
    PAL_FLAG = 13,
    PAL_SHADE = 14
};

struct Art {
    gs::Mipped ball;
    gs::Mipped shadow;
    gs::Mipped cup;
    gs::Mipped coin;
    gs::Mipped token;
    gs::Mipped flag;
    gs::Mipped golfer[2];
    gs::Mipped tree;
    gs::Mipped cloud;
    gs::Mipped sun;
    gs::Mipped tuft;
    gs::Mipped dot;
    gs::Mipped paper;
    gs::Mipped wood;
    gs::Mipped slot;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace putttape
