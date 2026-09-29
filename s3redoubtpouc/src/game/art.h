// Redoubt pictures. Drawn into sprite ROM at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace pouc {

enum Pal {
    PAL_HUD = 0,
    PAL_STONE = 1,
    PAL_RUNNER = 2,
    PAL_SENTRY = 3,
    PAL_IRON = 4,
    PAL_WATER = 5,
    PAL_TORCH = 6,
    PAL_POUCH = 7
};

struct Art {
    gs::Image runner;
    gs::Image sentry;
    gs::Image pouch;
    gs::Image bar;
    gs::Image gate;
    gs::Image block;
    gs::Image merlon;
    gs::Image plank;
    gs::Image water;
    gs::Image torch;
    gs::Image title;
    gs::Image hint;
    gs::Image win;
    gs::Image lose;
    gs::Image mark;
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace pouc
