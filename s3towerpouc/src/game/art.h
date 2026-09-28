// S3 TOWER POUC sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace tower {

enum Pal {
    PAL_HUD = 0,
    PAL_ASHLAR = 1,
    PAL_WALK = 2,
    PAL_POUCH = 3,
    PAL_WATCH = 4,
    PAL_COPPER = 5,
    PAL_FLAG = 6,
    PAL_LAMP = 7,
    PAL_ALERT = 8,
    PAL_VOID = 9,
    PAL_DOOR = 10,
    PAL_GO = 11
};

struct Art {
    gs::Mipped stand, runA, runB, duck, leap;
    gs::Mipped pouch[2];
    gs::Mipped merlon, banner, lamp, flame[2];
    gs::Mipped clock, hand, well, lip;
    gs::Mipped door, stair, shadow;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace tower
