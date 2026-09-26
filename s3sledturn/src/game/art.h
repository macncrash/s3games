// S3 SLED TURN sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace sledturn {

enum Pal : int {
    PAL_HUD = 0,
    PAL_SLED = 1,
    PAL_DOG = 2,
    PAL_RED = 3,
    PAL_BLUE = 4,
    PAL_TREE = 5,
    PAL_CABIN = 6,
    PAL_SNOW = 7,
    PAL_BIRD = 8,
    PAL_ALERT = 9,
    PAL_WIN = 10,
    PAL_BANNER = 11,
    PAL_TRAIL = 12,  // road generator: packed snow and ploughed walls
    PAL_SIGN = 13,
    PAL_SKY = 14,
    PAL_TAG = 15
};

constexpr int kPoses = 7;

struct Art {
    gs::Mipped sled[kPoses];
    gs::Mipped wreck;
    gs::Mipped dog[2];
    gs::Mipped stake;
    gs::Mipped sign[3];
    gs::Mipped spruce;
    gs::Mipped cabin;
    gs::Mipped cache;
    gs::Mipped post;
    gs::Mipped spray;
    gs::Mipped flake;
    gs::Mipped shadow;
    gs::Mipped raven[2];
    gs::Mipped sun;
    gs::Mipped cloud;
    gs::Mipped title;
    gs::Mipped upright;
    gs::Mipped tipped;
    gs::Mipped missed;
    gs::Mipped paused;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace sledturn
