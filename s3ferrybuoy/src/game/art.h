// S3 FERRY BUOY sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace ferry {

enum Pal : int {
    PAL_HUD = 0,
    PAL_FERRY = 1,
    PAL_NUN = 2,
    PAL_CAN = 3,
    PAL_YEL = 4,
    PAL_DOCK = 5,
    PAL_FOAM = 6,
    PAL_GULL = 7,
    PAL_WAVE = 8,
    PAL_CREW = 9,
    PAL_BANNER = 10,
    PAL_WIN = 11,
    PAL_ALERT = 12,
    PAL_SMOKE = 13,
    PAL_MAP = 14,
    PAL_ROCK = 15
};

struct Art {
    gs::Mipped ferry[16];
    gs::Mipped buoy[3];
    gs::Mipped quay, shed, crewShed, pile, ramp, crane, car, rock;
    gs::Mipped gull[2], foam, smoke, ring, lamp, pin, dot, panel;
    gs::Mipped title, sub, same, ahead, crewTook, wrong, paused;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace ferry
