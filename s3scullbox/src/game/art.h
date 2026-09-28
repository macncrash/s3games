// S3 SCULL BOX sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace scullbox {

enum Pal : int {
    PAL_HUD = 0,
    PAL_SHELL = 1,
    PAL_BOX = 2,
    PAL_DOCK = 3,
    PAL_REED = 4,
    PAL_FOAM = 5,
    PAL_WATER = 6
};

struct Art {
    gs::Mipped shell[16];  // 8 headings × catch/finish, bow is north at heading 0
    gs::Mipped post;
    gs::Mipped dash;
    gs::Mipped dock;
    gs::Mipped reed;
    gs::Mipped foam;
    int font[96] = {};
    int waterTile = 1;
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace scullbox
