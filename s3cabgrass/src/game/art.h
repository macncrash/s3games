// S3 CAB GRASS sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace cabgrass {

enum Pal : int {
    PAL_HUD = 0,
    PAL_CAB = 1,
    PAL_TREE = 2,
    PAL_LAMP = 3,
    PAL_WIN = 4,
    PAL_ALERT = 5,
    PAL_BANNER = 6,
    PAL_SMOKE = 7,
    PAL_POST = 8,
    PAL_STREET = 12,
    PAL_FIELD = 13
};

struct Art {
    gs::Mipped cab[8];
    gs::Mipped shade;
    gs::Mipped tree, lamp, post, tuft, meter;
    gs::Mipped smoke;
    gs::Mipped title, fullStop, onGrass;
    gs::Mipped ranOff, offGrass, shortStop, notFull, timed, legFail, paused;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace cabgrass
