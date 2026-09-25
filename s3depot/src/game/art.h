// S3 DEPOT pictures. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace depot {

enum Pal {
    PAL_HUD = 0,
    PAL_ALERT = 1,
    PAL_YARD = 2,
    PAL_BRICK = 3,
    PAL_NIGHT = 4,
    PAL_TUG = 5,
    PAL_CRATE = 6,
    PAL_DRUM = 7,
    PAL_PALLET = 8,
    PAL_LAMP = 9,
    PAL_TITLE = 10,
    PAL_DOCK = 11,
    PAL_RAIL = 12
};

struct Art {
    gs::Mipped tug[16];
    gs::Mipped shadow;
    gs::Mipped crate, drum, pallet;
    gs::Mipped lamp, bollard, moon, cloud, dust, beacon;
    gs::Mipped plaque;
    gs::Mipped logo, tag, hint, hint2, win, lose, stowed;
    int font[96] = {};
    int sky[4] = {};
    int conc[3] = {};
    int brick = 1, window = 1, roof = 1, post = 1;
    int hazard = 1, apron = 1, stripe = 1, stain = 1;
    int rail = 1, bar = 1;
    int doorTop = 1, doorBot = 1, shutTop = 1, shutBot = 1;
    int bayOpen = 1, bayFull = 1;
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace depot
