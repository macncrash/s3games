// S3 MARKET SEVEN pictures. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace marketseven {

enum Pal {
    PAL_HUD = 0,
    PAL_STALL = 1,
    PAL_CLERK = 2,
    PAL_RIVAL = 3,
    PAL_CREAM = 4,
    PAL_LOAF = 5,
    PAL_GOLD = 6,
    PAL_COIN = 7,
    PAL_OK = 8,
    PAL_WARN = 9,
    PAL_BAD = 10,
    PAL_DIM = 11,
    PAL_PIP = 12
};

struct Art {
    gs::Mipped awning, post, counter, clerk, buyer;
    gs::Mipped good[3];
    gs::Mipped coin[2];
    gs::Mipped dish, bell;
    gs::Image shade, bracket, solid;
    gs::Image digit[10];
    gs::Image title, seven, shortW, exact, winW, loseW;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace marketseven
