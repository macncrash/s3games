// S3 GOLF SEVEN pictures. Drawn at boot. No asset files.
#pragma once

#include "console/gfx.h"
#include "console/vdp.h"

namespace golfseven {

constexpr int kRace = 7;
constexpr float kGround = 176.f;
constexpr float kTee = 46.f;
constexpr float kBallR = 4.2f;

enum Pal {
    PAL_LINKS = 0,
    PAL_INK = 1,
    PAL_GOLD = 2,
    PAL_GREEN = 3,
    PAL_ALERT = 4,
    PAL_BALL = 5,
    PAL_FLAG = 6,
    PAL_MAN = 7,
    PAL_TURF = 8,
    PAL_HAZ = 9,
    PAL_SKY = 10,
    PAL_TITLE = 11,
    PAL_CUP = 12
};

struct Art {
    gs::Image ball;
    gs::Image flag;
    gs::Image man[2];
    gs::Image turf;
    gs::Image water;
    gs::Image sand;
    gs::Image cup;
    gs::Image cloud;
    gs::Image title;
    gs::Image win;
    gs::Image lose;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace golfseven
