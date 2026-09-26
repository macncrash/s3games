// S3 SKIFF MARK sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace skiffmark {

enum Pal : int {
    PAL_HUD = 0,
    PAL_HULL = 1,
    PAL_CREW = 2,
    PAL_REED = 3,
    PAL_WOOD = 4,
    PAL_MARK = 5,
    PAL_FOAM = 6,
    PAL_GULL = 7,
    PAL_BUOY = 8,
    PAL_MAP = 9,
    PAL_WIN = 10,
    PAL_ALERT = 11,
    PAL_BANNER = 12,
    PAL_SHORE = 13,
    PAL_BASIN = 14
};

struct Art {
    gs::Mipped hull[16];
    gs::Mipped disc, ring, spar, buoy, reed, post, plank, gull[2];
    gs::Mipped foam, dash, dot, pin, panel;
    gs::Mipped title, setDown, onMark, crewTook, missed, grounded, paused;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace skiffmark
