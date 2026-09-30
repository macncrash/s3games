// S3 KART LANE pictures. Drawn into VRAM at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace kartlane {

enum Pal : int {
    PAL_HUD = 0,
    PAL_KART = 1,
    PAL_CONE = 2,
    PAL_BALE = 3,
    PAL_POLE = 4,
    PAL_GATE = 5,
    PAL_DUST = 6,
    PAL_SKY = 7,
    PAL_ALERT = 8,
    PAL_WIN = 9,
    PAL_BANNER = 10,
    PAL_TAG = 11,
    PAL_LANE = 12,
    PAL_END = 13,
    PAL_SPARE = 15
};

struct Art {
    gs::Mipped kart[3];
    gs::Mipped wheel;
    gs::Mipped cone, bale, pole;
    gs::Mipped post, bar, flag;
    gs::Mipped dust, shadow;
    gs::Mipped sun, cloud;
    gs::Mipped title, held, whole, left, missed, crew, paused, end;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace kartlane
