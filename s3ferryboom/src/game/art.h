// S3 FERRY BOOM sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace fboom {

enum Pal : int {
    PAL_HUD = 0,
    PAL_FERRY = 1,
    PAL_DRIVE = 2,
    PAL_BOOM = 3,
    PAL_BANK = 4,
    PAL_FOAM = 5,
    PAL_GULL = 6,
    PAL_ALERT = 7,
    PAL_WIN = 8,
    PAL_BANNER = 9,
    PAL_DIM = 10,
};

struct Art {
    gs::Mipped hull[8];
    gs::Mipped drive[8];
    gs::Mipped post;
    gs::Mipped head;
    gs::Mipped bank;
    gs::Mipped foam;
    gs::Mipped gull[2];
    gs::Mipped title;
    gs::Mipped made;
    gs::Mipped missed;
    gs::Mipped paused;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace fboom
