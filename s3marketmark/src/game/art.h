// S3 MARKETMARK pictures. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace marketmark {

enum Pal {
    PAL_HUD = 0,
    PAL_STALL = 1,
    PAL_CLERK = 2,
    PAL_BUYER = 3,
    PAL_MARK = 4,
    PAL_GOODS = 5,
    PAL_COIN = 6,
    PAL_NOTE = 7,
    PAL_OK = 8,
    PAL_WARN = 9,
    PAL_BAD = 10,
    PAL_DIM = 11
};

struct Art {
    gs::Mipped awning, post, counter, clerk, buyer, hat, star;
    gs::Mipped pear, loaf, jar;
    gs::Mipped coin[4];
    gs::Mipped note, dish;
    gs::Image shade, bracket, solid;
    gs::Image digit[10];
    gs::Image title, sub, done, walked, exact, brief, heavy, markWord;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace marketmark
