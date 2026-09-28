// S3 SCULL SLIP pictures. Drawn into sprite ROM at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace scullslip {

enum Pal : int {
    PAL_HUD = 0,
    PAL_SHELL = 1,
    PAL_PIER = 2,
    PAL_PILE = 3,
    PAL_CREW = 4,
    PAL_MARK = 5,
    PAL_FOAM = 6,
    PAL_WATER = 7,
    PAL_REED = 8
};

struct Art {
    gs::Mipped shell[16];  // 8 headings × catch/finish. Bow is north at heading 0.
    gs::Mipped plank;
    gs::Mipped pile;
    gs::Mipped reed;
    gs::Mipped foam;
    gs::Mipped mark;
    gs::Mipped crew;
    int font[96] = {};
    int waterTile = 1;
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace scullslip
