// S3 FERRY SLIP sprites. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace ferryslip {

enum Pal : int {
    PAL_HUD = 0,
    PAL_FERRY = 1,
    PAL_RIVAL = 2,
    PAL_PIER = 3,
    PAL_POST = 4,
    PAL_FOAM = 5,
    PAL_CLOCK = 6,
    PAL_BANNER = 7,
    PAL_ALERT = 8,
    PAL_WIN = 9,
    PAL_DIM = 10,
};

struct Art {
    gs::Mipped hull[12];
    gs::Mipped rival[12];
    gs::Mipped pier;
    gs::Mipped post;
    gs::Mipped shed;
    gs::Mipped clock[6];
    gs::Mipped foam;
    gs::Mipped title;
    gs::Mipped berthed;
    gs::Mipped turned;
    gs::Mipped paused;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace ferryslip
