// S3 GOLF pictures. Drawn at boot. No asset files.
#pragma once

#include "console/gfx.h"
#include "console/vdp.h"

namespace golf {

enum Pal {
    PAL_WHITE = 0,
    PAL_GOLD = 1,
    PAL_RED = 2,
    PAL_GREEN = 3,
    PAL_BALL = 4,
    PAL_FLAG = 5,
    PAL_MAN = 6,
    PAL_WORLD = 7,
    PAL_CLOUD = 8,
    PAL_SUN = 9,
    PAL_AIM = 10,
    PAL_LOGO = 11,
    PAL_SHADOW = 12
};

struct Art {
    gs::Image course[3];
    gs::Mipped ball;
    gs::Mipped flag[2];
    gs::Mipped golfer[2];
    gs::Mipped cloud;
    gs::Mipped sun;
    gs::Mipped dot;
    gs::Mipped shadow;
    gs::Mipped logo;
    gs::Mipped win;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace golf
