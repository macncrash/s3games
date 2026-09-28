// S3 SCULL TURN sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace scullturn {

enum Pal : int {
    PAL_HUD = 0,
    PAL_ALERT = 1,
    PAL_WIN = 2,
    PAL_BANNER = 3,
    PAL_TAG = 4,
    PAL_SHELL = 5,
    PAL_POST = 6,
    PAL_REED = 7,
    PAL_HOUSE = 8,
    PAL_FLAG = 9,
    PAL_FOAM = 10,
    PAL_BIRD = 11,
    PAL_RIVER = 12,
    PAL_SKY = 13,
    PAL_MARK = 14,
};

constexpr int kPoses = 5;

struct Art {
    gs::Mipped shell[kPoses];
    gs::Mipped wreck;
    gs::Mipped post;
    gs::Mipped reed;
    gs::Mipped house;
    gs::Mipped flag;
    gs::Mipped mark[3];
    gs::Mipped wake;
    gs::Mipped heron[2];
    gs::Mipped sun;
    gs::Mipped cloud;
    gs::Mipped shadow;
    gs::Mipped title;
    gs::Mipped tipped;
    gs::Mipped missed;
    gs::Mipped made;
    gs::Mipped held;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace scullturn
