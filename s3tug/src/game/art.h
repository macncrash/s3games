// S3 TUG sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace tug {

enum Pal : int {
    PAL_HUD = 0,
    PAL_SHIP = 1,
    PAL_PILE = 2,
    PAL_DOCK = 3,
    PAL_BOX = 4,
    PAL_LANE = 5,
    PAL_FOAM = 6,
    PAL_GULL = 7,
    PAL_WIN = 8,
    PAL_ALERT = 9,
    PAL_BANNER = 10,
    PAL_MAP = 11,
    PAL_CRANE = 12,
};

struct Art {
    gs::Mipped ship[16];
    gs::Mipped pile;
    gs::Mipped dock;
    gs::Mipped crane;
    gs::Mipped dash;
    gs::Mipped foam;
    gs::Mipped gull[2];
    gs::Mipped dot;
    gs::Mipped panel;
    gs::Mipped title;
    gs::Mipped berthed;
    gs::Mipped piling;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace tug
