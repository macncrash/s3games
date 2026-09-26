// S3 TUGBOAT LANE sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace tuglane {

enum Pal : int {
    PAL_HUD = 0,
    PAL_TUG = 1,
    PAL_QUAY = 2,
    PAL_RED = 3,
    PAL_GREEN = 4,
    PAL_FOAM = 5,
    PAL_SMOKE = 6,
    PAL_GULL = 7,
    PAL_WIN = 8,
    PAL_ALERT = 9,
    PAL_BANNER = 10,
    PAL_LAMP = 11,
    PAL_LANE = 12,  // road generator: the buoyed channel
    PAL_MARK = 13,
    PAL_END = 14
};

struct Art {
    gs::Mipped tug[16];
    gs::Mipped shade;
    gs::Mipped buoyR, buoyG;
    gs::Mipped shed, house, crate, bollard, lamp, post;
    gs::Mipped line, foam, smoke, gull[2], pin;
    gs::Mipped title, held, left, missed, ranout, paused, gate;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace tuglane
