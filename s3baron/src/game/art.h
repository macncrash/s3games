// S3 BARON sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace baron {

enum Pal {
    PAL_HUD = 0,
    PAL_AMBER = 1,
    PAL_RED = 2,
    PAL_GREEN = 3,
    PAL_PLAYER = 4,
    PAL_ENEMY = 5,
    PAL_ACE = 6,
    PAL_FX = 7,
    PAL_PROP = 8,
    PAL_FIELD = 12
};

struct Art {
    gs::Mipped rear[3];
    gs::Mipped front[3];
    gs::Mipped balloon;
    gs::Mipped sight;
    gs::Mipped puff, flash, bullet, tracer, cloud;
    gs::Mipped prop[4];
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);
void skyFor(int wave, uint16_t& top, uint16_t& horizon, uint16_t& fog);

}  // namespace baron
