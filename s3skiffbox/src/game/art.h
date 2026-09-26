// S3 SKIFF BOX sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace skiffbox {

enum Pal : int {
    PAL_HUD = 0,
    PAL_HULL = 1,
    PAL_POST = 2,
    PAL_CLOCK = 3,
    PAL_WOOD = 4,
    PAL_FOAM = 5,
    PAL_GULL = 6,
    PAL_BANNER = 7,
    PAL_WIN = 8,
    PAL_ALERT = 9,
    PAL_WAVE = 10,
    PAL_TAG = 11,
    PAL_BOX = 12
};

struct Art {
    gs::Mipped hull[16];
    gs::Mipped hand[16];
    gs::Mipped face, tower, post, hdash, vdash;
    gs::Mipped pier, shed, flag, tuft, gull[2], foam, pin;
    gs::Mipped title, inBox, ahead, crewTook, paused, boxTag, crewTag;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace skiffbox
