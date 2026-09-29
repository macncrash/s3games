// S3 SUB LANE sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace sublane {

enum Pal : int {
    PAL_HUD = 0,
    PAL_SUB = 1,
    PAL_ROCK = 2,
    PAL_BUOY = 3,
    PAL_DOCK = 4,
    PAL_BUBBLE = 5,
    PAL_ALERT = 6,
    PAL_WIN = 7,
    PAL_ROAD = 12
};

struct Art {
    gs::Mipped sub, shade, screw;
    gs::Mipped rock[2];
    gs::Mipped buoy, dock, bubble;
    gs::Mipped title, stay, held, left, start;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace sublane
