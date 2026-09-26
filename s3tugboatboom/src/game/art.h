// S3 TUGBOAT BOOM sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace tugboom {

enum Pal : int {
    PAL_HUD = 0,
    PAL_TUG = 1,
    PAL_DRIVE = 2,
    PAL_BOOM = 3,
    PAL_QUAY = 4,
    PAL_FOAM = 5,
    PAL_END = 6,
    PAL_SMOKE = 7,
    PAL_WIN = 8,
    PAL_ALERT = 9,
    PAL_BANNER = 10,
    PAL_GULL = 11,
    PAL_CH = 12,
    PAL_MARK = 13
};

struct Art {
    gs::Mipped tug[16];
    gs::Mipped drive[16];
    gs::Mipped shade;
    gs::Mipped logH, logV, post, buoy, stripe, quay, shed, lamp;
    gs::Mipped foam, smoke, gull[2], pin, link;
    gs::Mipped title, delivered, missed, shortOf, offBoom, broke, paused;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace tugboom
