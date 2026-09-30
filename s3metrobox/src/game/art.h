// S3 METROBOX sprites and tiles. Everything is drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace metro {

enum Pal {
    PAL_HUD = 0,
    PAL_TRAIN = 1,
    PAL_RIVAL = 2,
    PAL_BOX = 3,
    PAL_TUNNEL = 4,
    PAL_LAMP = 5,
    PAL_SIGN = 6,
    PAL_CLOCK = 7
};

struct Art {
    gs::Mipped train;
    gs::Mipped rival;
    gs::Mipped post;
    gs::Mipped lamp;
    gs::Mipped person;
    gs::Mipped clock;
    gs::Mipped signal;
    int font[96] = {};
    int brick = 1;
    int brickHi = 1;
    int pipe = 1;
    int dark = 1;
    int farRail = 1;
    int nearRail = 1;
    int plat = 1;
    int platEdge = 1;
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace metro
