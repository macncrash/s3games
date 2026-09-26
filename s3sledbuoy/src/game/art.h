// S3 SLED BUOY sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace sled {

enum Pal : int {
    PAL_HUD = 0,
    PAL_SLED = 1,
    PAL_NUN = 2,
    PAL_CAN = 3,
    PAL_DOCK = 4,
    PAL_SPRAY = 5,
    PAL_BIRD = 6,
    PAL_TREE = 7,
    PAL_DRIFT = 8,
    PAL_OTHER = 9,
    PAL_BANNER = 10,
    PAL_WIN = 11,
    PAL_ALERT = 12,
    PAL_SNOW = 13,
    PAL_MAP = 14,
    PAL_LAMP = 15
};

struct Art {
    gs::Mipped sled[16];
    gs::Mipped nun, nun3, can;
    gs::Mipped quay, shed, pile, flag;
    gs::Mipped tree, drift, bird[2];
    gs::Mipped spray, ring, lamp, pin, dot, panel, crack;
    gs::Mipped title, round, same, made, missed, wrong, paused;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace sled
