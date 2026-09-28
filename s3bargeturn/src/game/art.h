// S3 BARGE TURN sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace bargeturn {

enum Pal : int {
    PAL_HUD = 0,
    PAL_ALERT = 1,
    PAL_WIN = 2,
    PAL_BANNER = 3,
    PAL_TAG = 4,
    PAL_HULL = 5,
    PAL_BUOY = 6,
    PAL_REED = 7,
    PAL_MILL = 8,
    PAL_CRANE = 9,
    PAL_FOAM = 10,
    PAL_BIRD = 11,
    PAL_RIVER = 12,
    PAL_SKY = 13,
    PAL_SIGN = 14,
};

constexpr int kPoses = 5;

struct Art {
    gs::Mipped barge[kPoses];
    gs::Mipped wreck;
    gs::Mipped buoy;
    gs::Mipped reed;
    gs::Mipped mill;
    gs::Mipped crane;
    gs::Mipped sign[3];
    gs::Mipped wake;
    gs::Mipped gull[2];
    gs::Mipped sun;
    gs::Mipped cloud;
    gs::Mipped shadow;
    gs::Mipped title;
    gs::Mipped tipped;
    gs::Mipped missed;
    gs::Mipped upright;
    gs::Mipped paused;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace bargeturn
