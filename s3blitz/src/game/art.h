// S3 BLITZ sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace blitz {

enum Pal {
    PAL_HUD = 0,
    PAL_SHIP = 1,
    PAL_TURRET = 2,
    PAL_CELL = 3,
    PAL_SPAR = 4,
    PAL_BLOCK = 5,
    PAL_BOLT = 6,
    PAL_BOOM = 7,
    PAL_PORT = 8,
    PAL_WALL = 9,
    PAL_AMBER = 10,
    PAL_BAD = 11,
    PAL_ROAD = 12,
    PAL_LOGO = 13,
    PAL_GOOD = 14,
    PAL_LAMP = 15
};

struct Art {
    gs::Mipped ship[3];
    gs::Mipped turret, spar, block, cell, bolt, boom, wall, lamp, shadow, port;
    gs::Mipped logo, mark;
    gs::Mipped digit[10];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace blitz
