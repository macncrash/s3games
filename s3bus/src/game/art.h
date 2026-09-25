// S3 BUS sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace bus {

enum Pal : int {
    PAL_HUD = 0,
    PAL_BUS = 1,
    PAL_BOX = 2,
    PAL_READY = 3,
    PAL_SHELTER = 4,
    PAL_CAR = 5,
    PAL_CAR2 = 6,
    PAL_TRUCK = 7,
    PAL_PERSON = 8,
    PAL_TREE = 9,
    PAL_ROAD = 10,
    PAL_ALERT = 11,
    PAL_OK = 12,
    PAL_DIM = 13,
    PAL_BANNER = 14,
    PAL_CONE = 15,
};

struct Art {
    gs::Mipped bus;
    gs::Mipped busYawL;
    gs::Mipped busYawR;
    gs::Mipped busDoor[2];
    gs::Mipped box;
    gs::Mipped shelter[6];
    gs::Mipped person[2];
    gs::Mipped car[2];
    gs::Mipped truck;
    gs::Mipped cone;
    gs::Mipped tree;
    gs::Mipped zebra;
    gs::Mipped shade;
    gs::Mipped panel;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace bus
