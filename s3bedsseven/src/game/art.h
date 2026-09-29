// S3 BEDS SEVEN pictures. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace bedsseven {

constexpr int GOAL = 7;
constexpr int BEDS = 3;

enum Pal {
    PAL_HUD = 0,
    PAL_GOLD = 1,
    PAL_WARN = 2,
    PAL_GOOD = 3,
    PAL_WOOD = 4,
    PAL_SOIL = 5,
    PAL_LEAF = 6,
    PAL_MAN = 7,
    PAL_RIVAL = 8,
    PAL_WATER = 9,
    PAL_YARD = 10,
    PAL_SKY = 11,
    PAL_PIP = 12,
    PAL_CAN = 13,
    PAL_DIM = 14,
    PAL_PATH = 15
};

struct Art {
    int font[96] = {};
    int grass = 0;
    int path = 0;
    gs::Mipped title;
    gs::Mipped sub;
    gs::Mipped win;
    gs::Mipped lose;
    gs::Mipped bed;
    gs::Mipped soil;
    gs::Mipped sprout[3];
    gs::Mipped man[2];
    gs::Mipped rival;
    gs::Mipped can;
    gs::Mipped drop;
    gs::Mipped pipOn;
    gs::Mipped pipOff;
    gs::Mipped shade;
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace bedsseven
