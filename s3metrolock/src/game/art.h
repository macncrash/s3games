// S3 METRO LOCK sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace metrolock {

enum Pal : int {
    PAL_HUD = 0,
    PAL_CAR = 1,
    PAL_CONCRETE = 2,
    PAL_GATE = 3,
    PAL_TILE = 4,
    PAL_LAMP = 5,
    PAL_RAIL = 6,
    PAL_WATER = 7,
    PAL_WIN = 8,
    PAL_ALERT = 9,
    PAL_GOLD = 10,
    PAL_SIGNAL = 11,
};

struct Art {
    gs::Mipped car;
    gs::Mipped nose;
    gs::Mipped gate;
    gs::Mipped concrete;
    gs::Mipped tile;
    gs::Mipped lamp;
    gs::Mipped sleeper;
    gs::Mipped signal;
    gs::Mipped drip;
    gs::Mipped title;
    gs::Mipped clear;
    gs::Mipped fail;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace metrolock
