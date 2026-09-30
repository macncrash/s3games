// S3 BUS GRASS sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace busgrass {

enum Pal : int {
    PAL_HUD = 0,
    PAL_BUS = 1,
    PAL_TREE = 2,
    PAL_LAMP = 3,
    PAL_WIN = 4,
    PAL_ALERT = 5,
    PAL_BANNER = 6,
    PAL_SMOKE = 7,
    PAL_POST = 8,
    PAL_BENCH = 9,
    PAL_STREET = 12,
    PAL_FIELD = 13
};

struct Art {
    gs::Mipped bus[8];
    gs::Mipped shade;
    gs::Mipped tree, lamp, post, tuft, chalk;
    gs::Mipped bench, shelter;
    gs::Mipped smoke;
    gs::Mipped title, fullStop, onGrass;
    gs::Mipped ranOff, offGrass, shortStop, notFull, curbStop, timed, legFail, paused;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace busgrass
