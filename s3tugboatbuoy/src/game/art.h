// S3 TUGBOAT BUOY sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace tug {

enum Pal : int {
    PAL_HUD = 0,
    PAL_TUG = 1,
    PAL_NUN = 2,
    PAL_CAN = 3,
    PAL_DOCK = 4,
    PAL_FOAM = 5,
    PAL_GULL = 6,
    PAL_ROCK = 7,
    PAL_WAVE = 8,
    PAL_BARGE = 9,
    PAL_BANNER = 10,
    PAL_WIN = 11,
    PAL_ALERT = 12,
    PAL_SMOKE = 13,
    PAL_MAP = 14,
    PAL_LAMP = 15
};

struct Art {
    gs::Mipped tug[16];
    gs::Mipped buoy[3];
    gs::Mipped quay, quayB, shed, pile, beam, crane;
    gs::Mipped barge, skiff, rock;
    gs::Mipped gull[2], foam, smoke, ring, lamp, pin, dot, panel;
    gs::Mipped title, sub, same, paused;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace tug
