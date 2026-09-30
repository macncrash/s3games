// S3 RAIL GRASS sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace railgrass {

enum Pal : int {
    PAL_HUD = 0,
    PAL_CAR = 1,
    PAL_TUFT = 2,
    PAL_RAIL = 3,
    PAL_WOOD = 4,
    PAL_MARK = 5,
    PAL_SPARK = 6,
    PAL_BIRD = 7,
    PAL_LAMP = 8,
    PAL_MAP = 9,
    PAL_WIN = 10,
    PAL_ALERT = 11,
    PAL_FIELD = 12,
    PAL_ENDF = 13,
    PAL_BANNER = 14
};

struct Art {
    gs::Mipped car[8];
    gs::Mipped tuft, sleeper, signal, flag, dash, halt, lamp;
    gs::Mipped bird[2];
    gs::Mipped spark, ring, dot, pin, panel;
    gs::Mipped title, fullStop, onGrass, missed, legFail, paused;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace railgrass
