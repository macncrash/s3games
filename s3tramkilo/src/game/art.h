// S3 TRAM KILO sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace tramkilo {

enum Pal : int {
    PAL_HUD = 0,
    PAL_AMBER = 1,
    PAL_BAD = 2,
    PAL_GOOD = 3,
    PAL_TRAM = 4,
    PAL_LORRY = 5,
    PAL_CAB = 6,
    PAL_OTHER = 7,
    PAL_BRICK = 8,
    PAL_STOP = 9,
    PAL_POLE = 10,
    PAL_WIRE = 11,
    PAL_SKY = 12,
    PAL_SIGN = 13,
};

struct Art {
    gs::Mipped tram;
    gs::Mipped wheel[3];
    gs::Mipped brick;
    gs::Mipped stop;
    gs::Mipped pole;
    gs::Mipped lamp;
    gs::Mipped cloud;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace tramkilo
