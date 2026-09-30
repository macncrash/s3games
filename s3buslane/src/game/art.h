// S3 BUS LANE sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace buslane {

enum Pal : int {
    PAL_HUD = 0,
    PAL_BUS = 1,
    PAL_SHELTER = 2,
    PAL_LAMP = 3,
    PAL_CAR = 4,
    PAL_DEPOT = 5,
    PAL_ALERT = 6,
    PAL_WIN = 7,
    PAL_ROAD = 12
};

struct Art {
    gs::Mipped bus, shade, stripe;
    gs::Mipped shelter, lamp, car, cone, depot;
    gs::Mipped title, stay, held, left, start;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace buslane
