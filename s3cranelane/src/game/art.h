// S3 CRANE LANE sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace cranelane {

enum Pal : int {
    PAL_HUD = 0,
    PAL_CRANE = 1,
    PAL_YARD = 2,
    PAL_CONE = 3,
    PAL_LIGHT = 4,
    PAL_GATE = 5,
    PAL_ALERT = 6,
    PAL_WIN = 7,
    PAL_ROAD = 12
};

struct Art {
    gs::Mipped crane, hook, shadow;
    gs::Mipped stack, barrier, cone, flood, gate;
    gs::Mipped title, stay, held, left, start;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace cranelane
