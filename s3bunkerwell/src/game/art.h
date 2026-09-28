// S3 BUNKER WELL sprites. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace bwell {

enum Pal {
    PAL_HUD = 0,
    PAL_YARD = 1,
    PAL_WELL = 2,
    PAL_YOU = 3,
    PAL_FOE = 4,
    PAL_SAP = 5,
    PAL_RAM = 6,
    PAL_FX = 7,
    PAL_AMBER = 8,
    PAL_ALERT = 9,
    PAL_OK = 10
};

constexpr float WELL_X = 160.f;
constexpr float WELL_Y = 118.f;

struct Art {
    gs::Mipped gunner[2];
    gs::Mipped raider[2];
    gs::Mipped sapper[2];
    gs::Mipped rammer[2];
    gs::Mipped well;
    gs::Mipped crack;
    gs::Mipped shot;
    gs::Mipped flash;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace bwell
