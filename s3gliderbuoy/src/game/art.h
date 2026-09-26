// S3 GLIDER BUOY sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace gbuoy {

enum Pal : int {
    PAL_HUD = 0,
    PAL_SAIL = 1,
    PAL_SPAR = 2,
    PAL_CONE = 3,
    PAL_DRUM = 4,
    PAL_DOCK = 5,
    PAL_FOAM = 6,
    PAL_GULL = 7,
    PAL_LIFT = 8,
    PAL_MAP = 9,
    PAL_SHADE = 10,
    PAL_BANNER = 11,
    PAL_WIN = 12,
    PAL_ALERT = 13,
    PAL_CREW = 14,
    PAL_WAVE = 15
};

struct Art {
    gs::Mipped wing[16];
    gs::Mipped spar, cone, drum;
    gs::Mipped dock, sock[3], hut, post, flag;
    gs::Mipped gull[2];
    gs::Mipped foam, ring, shade, pin, panel, dot, lift;
    gs::Mipped title, paused, sameDock, ahead, ditched, crewTook;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace gbuoy
