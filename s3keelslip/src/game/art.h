// S3 KEEL SLIP sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace keel {

enum Pal {
    PAL_HUD = 0,
    PAL_HULL = 1,
    PAL_PIER = 2,
    PAL_MARK = 3,
    PAL_WIN = 4,
    PAL_ALERT = 5,
    PAL_BANNER = 6,
    PAL_LAMP = 7,
    PAL_SLIP = 12,
    PAL_SHORE = 13
};

struct Art {
    gs::Mipped hull[8];
    gs::Mipped plank;
    gs::Mipped piling;
    gs::Mipped buoy;
    gs::Mipped foam;
    gs::Mipped lamp;
    gs::Mipped dot;
    gs::Mipped pin;
    gs::Mipped title;
    gs::Mipped berthed;
    gs::Mipped held;
    gs::Mipped tide;
    gs::Mipped scraped;
    gs::Mipped lost;
    gs::Mipped paused;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace keel
