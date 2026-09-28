// S3 CRANE LOCK sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace cranelock {

enum Pal {
    PAL_HUD = 0,
    PAL_AMBER = 1,
    PAL_RED = 2,
    PAL_GREEN = 3,
    PAL_HULL = 4,
    PAL_HOOK = 5,
    PAL_GATE = 6,
    PAL_STONE = 7,
    PAL_CREW = 8,
    PAL_MARK = 9
};

struct Art {
    gs::Mipped hull;
    gs::Mipped hook;
    gs::Mipped bead;
    gs::Mipped gate;
    gs::Mipped stone;
    gs::Mipped berth;
    gs::Mipped crew;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace cranelock
