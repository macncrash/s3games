// S3 SKIFF BOOM sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace skiffboom {

enum Pal : int {
    PAL_HUD = 0,
    PAL_HULL = 1,
    PAL_DRIVE = 2,
    PAL_RIVAL = 3,
    PAL_BOOM = 4,
    PAL_CRIB = 5,
    PAL_SHED = 6,
    PAL_BIRD = 7,
    PAL_FOAM = 8,
    PAL_MARK = 9,
    PAL_WIN = 10,
    PAL_ALERT = 11,
    PAL_BANNER = 12,
    PAL_WATER = 13,
    PAL_MAP = 14,
    PAL_CREW = 15
};

struct Art {
    gs::Mipped hull[16];
    gs::Mipped drive[16];
    gs::Mipped logV, logH, crib, buoy, pile, reed, shed, post, tender[2];
    gs::Mipped gull[2], heron, foam, dash, dot, panel;
    gs::Mipped title, delivered, onBoom, crewTook, driveLost, broke, missed, grounded, paused, boomSign, crewTag;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace skiffboom
