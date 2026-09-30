// S3 BUS BOOM sprites. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace busboom {

enum Pal : int {
    PAL_HUD = 0,
    PAL_BUS = 1,
    PAL_DRIVE = 2,
    PAL_BOOM = 3,
    PAL_WALK = 4,
    PAL_EXHAUST = 5,
    PAL_END = 6,
    PAL_LAMP = 7,
    PAL_WIN = 8,
    PAL_ALERT = 9,
    PAL_BANNER = 10,
    PAL_BIRD = 11,
    PAL_ROAD = 12,
    PAL_MARK = 13
};

struct Art {
    gs::Mipped bus[8];
    gs::Mipped drive[8];
    gs::Mipped shade;
    gs::Mipped girderH, girderV, post, stripe, walk, shelter, lamp;
    gs::Mipped exhaust, hitch;
    gs::Mipped bird[2];
    gs::Mipped title, delivered, missed, shortOf, offBoom, broke, paused;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace busboom
