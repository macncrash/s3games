// S3 MARKETBELL pictures. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace marketbell {

enum Pal {
    PAL_HUD = 0,
    PAL_STALL = 1,
    PAL_CLERK = 2,
    PAL_CUST = 3,
    PAL_GOODS = 4,
    PAL_COIN = 5,
    PAL_BELL = 6,
    PAL_LAMP = 7,
    PAL_OK = 8,
    PAL_WARN = 9,
    PAL_BAD = 10,
    PAL_DIM = 11,
    PAL_BOARD = 12
};

struct Art {
    gs::Mipped awning, post, counter, crate, basket;
    gs::Mipped clerk, buyer, bell, lamp, coin[4], dish;
    gs::Image shade, bracket, solid, digit[10];
    gs::Image title, sub, rung, dead, exact, shortw, overw;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace marketbell
