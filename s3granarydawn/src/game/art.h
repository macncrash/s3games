// S3 GRANARY DAWN sprites. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace granarydawn {

enum Pal {
    PAL_HUD = 0,
    PAL_YARD = 1,
    PAL_BARN = 2,
    PAL_YOU = 3,
    PAL_CUP = 4,
    PAL_FIRE = 5,
    PAL_MOON = 6,
    PAL_STAR = 7,
    PAL_SACK = 8,
    PAL_GOLD = 9,
    PAL_ALERT = 10,
    PAL_OK = 11,
    PAL_CHAFF = 12
};

constexpr int kFlares = 4;
constexpr float FLARE_X[kFlares] = {46.f, 108.f, 212.f, 274.f};
constexpr float FLARE_Y = 168.f;
constexpr float GROUND_Y = 188.f;

struct Art {
    gs::Mipped barn;
    gs::Mipped silo;
    gs::Mipped sack;
    gs::Mipped cup;
    gs::Mipped flame[2];
    gs::Mipped keeper[2];
    gs::Mipped moon;
    gs::Mipped sun;
    gs::Mipped star;
    gs::Mipped chaff;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace granarydawn
