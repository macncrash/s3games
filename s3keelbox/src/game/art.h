// S3 KEEL BOX sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace keelbox {

enum Pal : int {
    PAL_HUD = 0,
    PAL_BOAT = 1,
    PAL_SHORE = 2,
    PAL_MARK = 3,
    PAL_END = 4,
    PAL_FOAM = 5,
    PAL_SAIL = 6,
    PAL_BIRD = 7,
    PAL_WIN = 8,
    PAL_ALERT = 9,
    PAL_BANNER = 10,
    PAL_WIND = 11,
    PAL_CH = 12
};

struct Art {
    gs::Mipped boat[12];
    gs::Mipped shade;
    gs::Mipped committee, pin, post, hbar, vbar, hatch;
    gs::Mipped reed, vane, foam, gull[2], arrow;
    gs::Mipped title, stopped, missed, outside, stoppedShort, paused;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace keelbox
