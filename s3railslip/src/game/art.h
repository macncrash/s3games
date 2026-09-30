// S3 RAIL SLIP sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace railslip {

enum Pal {
    PAL_HUD = 0,
    PAL_BOAT = 1,
    PAL_QUAY = 2,
    PAL_PILE = 3,
    PAL_FLAG = 4,
    PAL_GULL = 5,
    PAL_ALERT = 6,
    PAL_WATER = 12
};

struct Art {
    gs::Mipped boat;
    gs::Mipped sleeper;
    gs::Mipped pile;
    gs::Mipped shed;
    gs::Mipped gull;
    gs::Mipped flag;
    gs::Mipped lamp;
    gs::Mipped glyph[96];
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace railslip
