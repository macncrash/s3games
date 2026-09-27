// S3 CAB TURN sprites. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace cabturn {

enum Pal {
    PAL_HUD = 0,
    PAL_CAB = 1,
    PAL_ROAD = 2,
    PAL_BLOCK = 3,
    PAL_WIN = 4,
    PAL_ALERT = 5,
    PAL_BANNER = 6,
    PAL_CREW = 7,
    PAL_MARK = 8
};

struct Art {
    gs::Mipped cab[8];
    gs::Mipped road, dash, block, lamp, cone, flag, wheel;
    gs::Mipped title, cleared, tipped, crew, street, paused;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace cabturn
