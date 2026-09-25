// S3 KEEL sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace keel {

enum Pal : int {
    PAL_HUD = 0,
    PAL_BOAT = 1,
    PAL_MARK0 = 2,
    PAL_MARK1 = 3,
    PAL_MARK2 = 4,
    PAL_DOCK = 5,
    PAL_FOAM = 6,
    PAL_GULL = 7,
    PAL_MAP = 8,
    PAL_ROCK = 9,
    PAL_WAVE = 10,
    PAL_BANNER = 11,
    PAL_WIN = 12,
    PAL_ALERT = 13,
};

struct Art {
    gs::Mipped boat[16][2];
    gs::Mipped buoy[3];
    gs::Mipped dock;
    gs::Mipped foam;
    gs::Mipped gull[2];
    gs::Mipped rock;
    gs::Mipped dot;
    gs::Mipped pin;
    gs::Mipped ring;
    gs::Mipped pile;
    gs::Mipped wind;
    gs::Mipped panel;
    gs::Mipped title;
    gs::Mipped docked;
    gs::Mipped closed;
    gs::Mipped paused;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace keel
