// S3 BIKE SLIP pictures. Drawn into VRAM at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace slip {

enum Pal {
    PAL_HUD = 0,
    PAL_BIKE = 1,
    PAL_POST = 2,
    PAL_BUOY = 3,
    PAL_TITLE = 4,
    PAL_ALERT = 5,
    PAL_ROAD = 12
};

struct Art {
    gs::Mipped bike[3];
    gs::Mipped shadow;
    gs::Mipped post;
    gs::Mipped buoy;
    gs::Mipped mouth;
    gs::Image title;
    gs::Image sub;
    gs::Image rule;
    gs::Image berthed;
    gs::Image missed;
    gs::Image tide;
    gs::Image edged;
    int font[128] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace slip
