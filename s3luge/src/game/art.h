// S3 LUGE pictures. Drawn into VRAM at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace luge {

enum Pal {
    PAL_HUD = 0,
    PAL_RIDER = 1,
    PAL_POST = 2,
    PAL_CHECK = 3,
    PAL_FX = 4,
    PAL_TITLE = 5,
    PAL_CANYON = 6,
    PAL_GOLD = 7,
    PAL_ALERT = 8,
    PAL_BANNER = 9,
    PAL_ICE = 12
};

struct Art {
    gs::Mipped flat, tuck, lean;
    gs::Mipped post, beam;
    gs::Mipped dropBan, finishBan;
    gs::Mipped spark, shadow;
    gs::Image title, sub;
    gs::Image num[3];
    gs::Image clean, hit, slow;
    int font[128] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace luge
