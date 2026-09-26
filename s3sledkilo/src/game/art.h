// S3 SLED KILO sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace sledkilo {

enum Pal : int {
    PAL_HUD = 0,
    PAL_SLED = 1,
    PAL_WAGON = 2,
    PAL_BIKE = 3,
    PAL_ROAD = 4,
    PAL_PINE = 5,
    PAL_CLOCK = 6,
    PAL_SPRAY = 7,
    PAL_CREW = 8,
    PAL_BANNER = 9,
    PAL_WIN = 10,
    PAL_ALERT = 11,
    PAL_POST = 12,
    PAL_TAG = 13
};

struct Art {
    gs::Mipped sled[16];
    gs::Mipped wheel[4][8];
    gs::Mipped hay, cart, bike, dray;
    gs::Mipped pine, cabin, lamp, post, ribbon, startLine, spray, dot;
    gs::Mipped clock[8];
    gs::Mipped bird[2];
    gs::Mipped title, paused, kilometer, clean, touched, leftSnow, crewTook;
    gs::Mipped m250, m500, m750, m1000, crewTag;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace sledkilo
