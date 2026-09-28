// S3 ALLEY POUC sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace alley {

enum Pal {
    PAL_HUD = 0,
    PAL_BRICK = 1,
    PAL_STONE = 2,
    PAL_POUCH = 3,
    PAL_PLAYER = 4,
    PAL_IRON = 5,
    PAL_WOOD = 6,
    PAL_LAMP = 7,
    PAL_ALERT = 8,
    PAL_DRAIN = 9,
    PAL_DOOR = 10,
    PAL_GO = 11
};

struct Art {
    gs::Mipped stand, runA, runB, duck, leap;
    gs::Mipped pouch[2];
    gs::Mipped bin, lamp, flame[2];
    gs::Mipped grate, lip, line, peg;
    gs::Mipped door, stair, shadow;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace alley
