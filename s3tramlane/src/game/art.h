// S3 TRAM LANE sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace tramlane {

enum Pal : int {
    PAL_HUD = 0,
    PAL_TRAM = 1,
    PAL_STOP = 2,
    PAL_WIRE = 3,
    PAL_POLE = 4,
    PAL_SIGN = 5,
    PAL_ALERT = 6,
    PAL_WIN = 7,
    PAL_ROAD = 12
};

struct Art {
    gs::Mipped tram, shade, pant;
    gs::Mipped stop[2];
    gs::Mipped pole, wire, gate, post;
    gs::Mipped title, stay, held, left, start;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace tramlane
