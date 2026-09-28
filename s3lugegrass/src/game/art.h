// S3 LUGE GRASS sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace lugegrass {

constexpr float kHalf = 3.3f;
constexpr float kGrassL = 92.f;
constexpr float kGrassR = 124.f;
constexpr float kPpm = 6.2f;

enum Pal : int {
    PAL_HUD = 0,
    PAL_LUGE = 1,
    PAL_ICE = 2,
    PAL_GRASS = 3,
    PAL_DIRT = 4,
    PAL_PINE = 5,
    PAL_POST = 6,
    PAL_TUFT = 7,
    PAL_FLAG = 8,
    PAL_WIN = 9,
    PAL_ALERT = 10,
    PAL_BANNER = 11,
    PAL_HUT = 12,
    PAL_BIRD = 13,
    PAL_SPRAY = 14
};

struct Art {
    gs::Mipped luge;
    gs::Mipped spray;
    gs::Mipped ice;
    gs::Mipped grass;
    gs::Mipped dirt;
    gs::Mipped pine;
    gs::Mipped post;
    gs::Mipped tuft;
    gs::Mipped flag;
    gs::Mipped hut;
    gs::Mipped bird[2];
    gs::Mipped title;
    gs::Mipped grassWord;
    gs::Mipped stopped;
    gs::Mipped missed;
    gs::Mipped shortStop;
    gs::Mipped paused;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace lugegrass
