// S3 BARGE PLAT sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace bargeplat {

enum Pal {
    PAL_HUD = 0,
    PAL_AMBER = 1,
    PAL_BAD = 2,
    PAL_GOOD = 3,
    PAL_HULL = 4,
    PAL_QUAY = 5,
    PAL_WATER = 6,
    PAL_PILE = 7,
    PAL_BANK = 8,
    PAL_SKY = 9,
    PAL_MARK = 10,
    PAL_CRANE = 11,
    PAL_FOAM = 12,
    PAL_BIRD = 13,
    PAL_CRATE = 14,
    PAL_WAKE = 15
};

struct Art {
    gs::Mipped hull, crate, plank, stripe, pile;
    gs::Mipped water, bank, cloud, sun;
    gs::Mipped crane, lamp, foam, wake;
    gs::Mipped bird[2];
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace bargeplat
