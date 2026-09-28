// S3 SCULL GRASS sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace scullgrass {

enum Pal : int {
    PAL_HUD = 0,
    PAL_SHELL = 1,
    PAL_GRASS = 2,
    PAL_BANK = 3,
    PAL_REED = 4,
    PAL_DOCK = 5,
    PAL_FOAM = 6,
    PAL_WATER = 7,
    PAL_STAKE = 8
};

struct Art {
    gs::Mipped shell[16];  // 8 headings × catch/finish, bow is north at heading 0
    gs::Mipped mat;
    gs::Mipped tuft;
    gs::Mipped reed;
    gs::Mipped dock;
    gs::Mipped stake;
    gs::Mipped foam;
    gs::Mipped tree;
    int font[96] = {};
    int waterTile = 1;
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace scullgrass
