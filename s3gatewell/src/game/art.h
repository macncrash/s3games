// S3 GATE WELL sprites. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace well {

enum Pal {
    PAL_TEXT = 0,
    PAL_GOLD = 1,
    PAL_ALERT = 2,
    PAL_GOOD = 3,
    PAL_STONE = 4,
    PAL_WOOD = 5,
    PAL_KEEPER = 6,
    PAL_RAIDER = 7,
    PAL_RAM = 8,
    PAL_YARD = 9,
    PAL_FX = 10,
    PAL_SKY = 11
};

// Three wickets, stacked. Feet sit on the path.
inline float laneFoot(int lane) { return 88.f + float(lane) * 44.f; }

struct Art {
    gs::Mipped well;
    gs::Mipped rubble;
    gs::Mipped crack;
    gs::Mipped bucket;
    gs::Mipped gate;
    gs::Mipped keeper[2];
    gs::Mipped runner[2];
    gs::Mipped club[2];
    gs::Mipped ram[2];
    gs::Mipped puff;
    gs::Mipped shock;
    gs::Mipped bar;
    gs::Mipped shadow;
    gs::Mipped moon;
    gs::Mipped star;
    gs::Mipped chain;
    gs::Mipped winch;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace well
