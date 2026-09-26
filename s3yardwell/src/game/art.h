// S3 YARD WELL sprites. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace yardwell {

// Painted yard and sprites share this anchor (tile column 20, row 16).
constexpr float kWellX = 160.f;
constexpr float kWellY = 130.f;

enum Pal {
    PAL_TEXT = 0,
    PAL_GOLD = 1,
    PAL_ALERT = 2,
    PAL_GOOD = 3,
    PAL_KEEPER = 4,
    PAL_MOLE = 5,
    PAL_GOAT = 6,
    PAL_BULL = 7,
    PAL_WELL = 8,
    PAL_PROP = 9,
    PAL_FX = 10,
    PAL_GROUND = 11,
    PAL_SKY = 12
};

struct Art {
    gs::Mipped well;
    gs::Mipped rubble;
    gs::Mipped crack;
    gs::Mipped keeper[3];
    gs::Mipped mole[2];
    gs::Mipped goat[2];
    gs::Mipped bull[2];
    gs::Mipped puff;
    gs::Mipped whoosh;
    gs::Mipped bar;
    gs::Mipped shadow;
    gs::Mipped tree;
    gs::Mipped shed;
    gs::Mipped barrow;
    gs::Mipped can;
    gs::Mipped shirt;
    gs::Mipped rope;
    gs::Mipped bush;
    gs::Mipped bird[2];
    gs::Mipped cloud;
    gs::Mipped sun;
    gs::Mipped glyph[96];
    int font[96] = {};
    int panel = 0;
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace yardwell
