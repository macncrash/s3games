// S3 METROBOOM pictures. Drawn into VRAM at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace metro {

enum Pal {
    PAL_HUD = 0,
    PAL_CAR = 1,
    PAL_CREW = 2,
    PAL_DRIVE = 3,
    PAL_BOOM = 4,
    PAL_TUNNEL = 5,
    PAL_LAMP = 6,
    PAL_GATE = 7
};

struct Art {
    gs::Mipped car;
    gs::Mipped crew;
    gs::Mipped drive;
    gs::Mipped boom;
    gs::Mipped hook;
    gs::Mipped lamp;
    gs::Mipped gate;
    int font[96] = {};
    int arch = 1;
    int seam = 1;
    int cable = 1;
    int voidTile = 1;
    int rail = 1;
    int sleeper = 1;
    int deck = 1;
    int lip = 1;
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace metro
