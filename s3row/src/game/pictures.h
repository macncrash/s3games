// S3 ROW sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace row {

enum Pal {
    PAL_HUD = 0,
    PAL_SHELL = 1,
    PAL_BUOY = 2,
    PAL_WAKE = 3,
    PAL_LAUNCH = 4,
    PAL_BANNER = 5,
    PAL_TITLE = 6,
    PAL_SKY = 7,
    PAL_AMBER = 8,
    PAL_ALERT = 9,
    PAL_GOOD = 10,
    PAL_SCENE = 11,
    PAL_ROAD = 12,
    PAL_DIM = 15
};

struct Art {
    gs::Mipped shell[4];
    gs::Mipped buoy;
    gs::Mipped wake;
    gs::Mipped splash;
    gs::Mipped shade;
    gs::Mipped launch;
    gs::Mipped tree[2];
    gs::Mipped house;
    gs::Mipped stand;
    gs::Mipped cloud;
    gs::Mipped sun;
    gs::Mipped bird;
    gs::Mipped sign[5];
    gs::Mipped banner;
    gs::Mipped title;
    gs::Mipped sub;
    gs::Mipped stay;
    gs::Mipped win;
    gs::Mipped out;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace row
