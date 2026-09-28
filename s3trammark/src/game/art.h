// S3 TRAM MARK sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace trammark {

enum Pal : int {
    PAL_HUD = 0,
    PAL_TRAM = 1,
    PAL_TOWN = 2,
    PAL_BALLAST = 3,
    PAL_MARK = 4,
    PAL_END = 5,
    PAL_ALERT = 6,
    PAL_WIN = 7,
    PAL_RAIL = 8,
    PAL_WIRE = 9
};

struct Art {
    gs::Mipped tram[16];
    gs::Mipped shade;
    gs::Mipped block[3];
    gs::Mipped pole;
    gs::Mipped ballast, sleeper, rail, mark, ring, buffer, tree, plat;
    gs::Mipped title, set, missed, hold, start;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace trammark
