// S3 FOUNDRY WELL sprites. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace foundrywell {

constexpr float kWellX = 160.f;
constexpr float kWellY = 126.f;

enum Pal {
    PAL_TEXT = 0,
    PAL_EMBER = 1,
    PAL_ALERT = 2,
    PAL_GOOD = 3,
    PAL_SMITH = 4,
    PAL_SPARK = 5,
    PAL_SLAG = 6,
    PAL_POUR = 7,
    PAL_WELL = 8,
    PAL_IRON = 9,
    PAL_FX = 10,
    PAL_FLOOR = 11,
    PAL_HEAT = 12
};

struct Art {
    gs::Mipped well;
    gs::Mipped rubble;
    gs::Mipped crack;
    gs::Mipped smith[3];
    gs::Mipped spark[2];
    gs::Mipped slag[2];
    gs::Mipped pour[2];
    gs::Mipped puff;
    gs::Mipped arc;
    gs::Mipped bar;
    gs::Mipped shadow;
    gs::Mipped stack;
    gs::Mipped crucible;
    gs::Mipped anvil;
    gs::Mipped ladle;
    gs::Mipped ingot;
    gs::Mipped chain;
    gs::Mipped glyph[96];
    int font[96] = {};
    int panel = 0;
    int brick = 0;
    int soot = 0;
    int plate = 0;
    int grate = 0;
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace foundrywell
