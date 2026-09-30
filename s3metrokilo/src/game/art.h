// S3 METROKILO pictures. Drawn into VRAM at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace metro {

enum Pal {
    PAL_HUD = 0,
    PAL_ALERT = 1,
    PAL_TITLE = 2,
    PAL_CAR = 3,
    PAL_WHEEL = 4,
    PAL_HANG = 5,
    PAL_CREW = 6,
    PAL_CLOCK = 7,
    PAL_TUNNEL = 8
};

struct Art {
    gs::Mipped car[2];
    gs::Mipped duck;
    gs::Mipped spill;
    gs::Mipped wheel[2];
    gs::Mipped hang;
    gs::Mipped crew;
    gs::Mipped clock[8];
    gs::Mipped lamp;
    gs::Image title;
    gs::Image sub;
    int font[96] = {};
    int wall = 1;
    int glow = 2;
    int sleeper = 3;
    int rail = 4;
    int pipe = 5;
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace metro
