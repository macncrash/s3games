// S3 TUGBOAT KILO sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace tugkilo {

enum Pal : int {
    PAL_HUD = 0,
    PAL_TUG = 1,
    PAL_MILL = 2,
    PAL_PADDLE = 3,
    PAL_DRUM = 4,
    PAL_QUAY = 5,
    PAL_GATE = 6,
    PAL_FOAM = 7,
    PAL_SMOKE = 8,
    PAL_GULL = 9,
    PAL_BANNER = 10,
    PAL_WIN = 11,
    PAL_ALERT = 12,
    PAL_TAG = 13
};

struct Art {
    gs::Mipped tug[16];
    gs::Mipped mill[8];
    gs::Mipped paddle[8];
    gs::Mipped drum[8];
    gs::Mipped quay, house, pier, barge, crane;
    gs::Mipped post, spar, buoy, line, foam, smoke, gull[2];
    gs::Mipped title, made, clean, missed, touched, cut, paused;
    gs::Mipped m250, m500, m750, m1000, endTag;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace tugkilo
