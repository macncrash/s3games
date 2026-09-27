// S3 FERRY TURN sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace ferryturn {

enum Pal {
    PAL_HUD = 0,
    PAL_AMBER = 1,
    PAL_BAD = 2,
    PAL_GOOD = 3,
    PAL_HULL = 4,
    PAL_BUOY = 5,
    PAL_DOCK = 6,
    PAL_WAKE = 7,
    PAL_CAR = 8,
    PAL_SHORE = 9,
    PAL_GULL = 10,
    PAL_FUNNEL = 11,
    PAL_DECK = 12,
    PAL_POST = 13,
    PAL_SLIP = 14,
    PAL_MARK = 15
};

struct Hull {
    gs::Mipped img;
};

struct Art {
    Hull hull[7];
    gs::Mipped buoy, dock, post, car, wake, gull, slip, chev;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace ferryturn
