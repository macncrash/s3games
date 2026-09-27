// S3 CAB LANE sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace cablane {

enum Pal : int {
    PAL_HUD = 0,
    PAL_CAB = 1,
    PAL_BLOCK = 2,
    PAL_PARK = 3,
    PAL_LAMP = 4,
    PAL_SIGN = 5,
    PAL_ALERT = 6,
    PAL_WIN = 7,
    PAL_ROAD = 12
};

struct Art {
    gs::Mipped cab, shade;
    gs::Mipped block[3];
    gs::Mipped park, lamp, gate, meter;
    gs::Mipped title, stay, held, left, start;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace cablane
