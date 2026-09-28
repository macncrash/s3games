// S3 TRAM TURN sprites. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace tramturn {

enum Pal {
    PAL_HUD = 0,
    PAL_TRAM = 1,
    PAL_RAIL = 2,
    PAL_YARD = 3,
    PAL_WIN = 4,
    PAL_ALERT = 5,
    PAL_BANNER = 6,
    PAL_CREW = 7,
    PAL_WIRE = 8
};

struct Art {
    gs::Mipped tram[8];
    gs::Mipped rail, sleeper, pole, wire, shed, clock, flag;
    gs::Mipped title, cleared, tipped, crew, rails, paused;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace tramturn
