// S3 BARGE sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace barge {

enum Pal : int {
    PAL_HUD = 0,
    PAL_BOAT = 1,
    PAL_STONE = 2,
    PAL_GATE = 3,
    PAL_GRASS = 4,
    PAL_HOUSE = 5,
    PAL_WATER = 6,
    PAL_FOAM = 7,
    PAL_TREE = 8,
    PAL_WIN = 9,
    PAL_ALERT = 10,
    PAL_BANNER = 11,
    PAL_GULL = 12,
    PAL_FENDER = 13,
    PAL_SUN = 14,
};

struct Art {
    gs::Mipped boat;
    gs::Mipped gate;
    gs::Mipped stone;
    gs::Mipped grass;
    gs::Mipped house;
    gs::Mipped tree[2];
    gs::Mipped post;
    gs::Mipped gull[2];
    gs::Mipped water;
    gs::Mipped foam;
    gs::Mipped sluice;
    gs::Mipped gauge;
    gs::Mipped sun;
    gs::Mipped cloud;
    gs::Mipped bar;
    gs::Mipped puck;
    gs::Mipped title;
    gs::Mipped clear;
    gs::Mipped scraped;
    gs::Mipped cill;
    gs::Mipped timeUp;
    gs::Mipped paused;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace barge
