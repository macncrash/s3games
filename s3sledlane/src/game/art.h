// S3 SLED LANE sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace sledlane {

enum Pal : int {
    PAL_HUD = 0,
    PAL_SLED = 1,
    PAL_DOG = 2,
    PAL_STAKE = 3,
    PAL_TREE = 4,
    PAL_HUT = 5,
    PAL_SNOW = 6,
    PAL_BIRD = 7,
    PAL_ALERT = 8,
    PAL_WIN = 9,
    PAL_BANNER = 10,
    PAL_TAG = 11,
    PAL_LANE = 12,  // road generator: packed lane and snow walls
    PAL_END = 13,
    PAL_SKY = 14,
    PAL_SPARE = 15
};

struct Art {
    gs::Mipped sled[3];
    gs::Mipped dog[2];
    gs::Mipped stakeRed, stakeBlue;
    gs::Mipped spruce, hut, cache;
    gs::Mipped post, bar, flag;
    gs::Mipped spray, flake, bead, shadow;
    gs::Mipped raven[2];
    gs::Mipped sun, cloud, moon;
    gs::Mipped title, held, whole, left, missed, paused, end;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace sledlane
