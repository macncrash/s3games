// S3 SLED GRASS sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace sledgrass {

enum Pal : int {
    PAL_HUD = 0,
    PAL_TEAM = 1,
    PAL_PINE = 2,
    PAL_WOOD = 3,
    PAL_TUFT = 4,
    PAL_MARK = 5,
    PAL_RAVEN = 6,
    PAL_DRIFT = 7,
    PAL_SPRAY = 8,
    PAL_MAP = 9,
    PAL_WIN = 10,
    PAL_ALERT = 11,
    PAL_SNOW = 12,
    PAL_FIELD = 13,
    PAL_END = 14,
    PAL_BANNER = 15
};

struct Art {
    gs::Mipped team[16];
    gs::Mipped pine, cabin, tuft, drift, flag, post, rail;
    gs::Mipped raven[2];
    gs::Mipped spray, ring, pin, dot, panel;
    gs::Mipped title, fullStop, onGrass, missed, offGrass, timed, legFail, paused, endWord;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace sledgrass
