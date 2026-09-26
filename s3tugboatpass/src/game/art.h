// S3 TUGBOAT PASS sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace tugpass {

enum Pal : int {
    PAL_HUD = 0,
    PAL_AMBER = 1,
    PAL_RED = 2,
    PAL_GREEN = 3,
    PAL_TUG = 4,
    PAL_RIVAL = 5,
    PAL_CLIFF = 6,
    PAL_PINE = 7,
    PAL_HUT = 8,
    PAL_FX = 9,
    PAL_BIRD = 10,
    PAL_STORM = 11
};

constexpr int YAWS = 16;
constexpr int TUG_PX = 70;  // nose-to-stern pixels in the unrotated tug

struct Art {
    gs::Mipped tug[YAWS];
    gs::Mipped face, ridge, pine, hut;
    gs::Mipped buoy, beacon, daymark;
    gs::Mipped gull[2];
    gs::Mipped smoke, wake, foam, shadow;
    gs::Mipped cloud, rain, bolt;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace tugpass
