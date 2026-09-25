// S3 KEEP sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace keep {

enum Pal {
    PAL_HUD = 0,
    PAL_STONE = 1,
    PAL_OAK = 2,
    PAL_IRON = 3,
    PAL_NIGHT = 4,
    PAL_FIRE = 5,
    PAL_CLOAK = 6,
    PAL_GOLD = 7,
    PAL_RED = 8,
    PAL_TRACK = 9,
    PAL_WARN = 10
};

struct Art {
    gs::Mipped door, jamb, lintel, floor;
    gs::Mipped bar, barTilt, bracket;
    gs::Mipped keeper, keeperShove;
    gs::Mipped hand, ram, crow;
    gs::Mipped torch[2];
    gs::Mipped night, crack, mote, chip;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace keep
