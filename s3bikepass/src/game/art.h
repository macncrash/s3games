// S3 BIKE PASS pictures. Drawn into VRAM at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace bikepass {

enum Pal {
    PAL_INK = 0,
    PAL_BIKE = 1,
    PAL_PEAK = 2,
    PAL_LOG = 3,
    PAL_DUST = 4,
    PAL_TITLE = 5,
    PAL_GATE = 6,
    PAL_BAD = 7,
    PAL_SKY = 8,
    PAL_HOT = 9,
    PAL_GOOD = 10,
    PAL_ROAD = 12
};

struct Art {
    gs::Mipped bike[5];
    gs::Mipped peak;
    gs::Mipped log;
    gs::Mipped dust;
    gs::Mipped gateOpen;
    gs::Mipped gateShut;
    gs::Image title;
    gs::Image sub;
    gs::Image cleared;
    gs::Image ditch;
    gs::Image late;
    gs::Image paused;
    int font[128] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace bikepass
