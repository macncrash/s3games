// S3 FLAK sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace flak {

enum Pal {
    PAL_WHITE = 0,
    PAL_AMBER = 1,
    PAL_RED = 2,
    PAL_GREEN = 3,
    PAL_SHIP = 4,
    PAL_PLANE = 5,
    PAL_BURST = 6,
    PAL_GUN = 7,
    PAL_SKY = 8,
    PAL_SEA = 9
};

struct Art {
    gs::Mipped bomber[2];
    gs::Mipped barrel[9];
    gs::Mipped mount;
    gs::Mipped deck;
    gs::Mipped rail;
    gs::Mipped bow;
    gs::Mipped funnel;
    gs::Mipped crate;
    gs::Mipped ring;
    gs::Mipped round;
    gs::Mipped sight;
    gs::Mipped tracer;
    gs::Mipped puff;
    gs::Mipped flash;
    gs::Mipped smoke;
    gs::Mipped bomb;
    gs::Mipped cloud;
    gs::Mipped sun;
    gs::Mipped bird[2];
    gs::Mipped wave;
    gs::Mipped gate;
    gs::Mipped freighter;
    gs::Mipped crack;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

// Barrel bitmaps are authored at these degrees from straight up.
extern const float kBarrelDeg[9];

}  // namespace flak
