// S3 TRENCH DAWN sprites. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace trench {

enum Pal {
    PAL_HUD = 0,
    PAL_EARTH = 1,
    PAL_WOOD = 2,
    PAL_YOU = 3,
    PAL_IRON = 4,
    PAL_FIRE = 5,
    PAL_MOON = 6,
    PAL_STAR = 7,
    PAL_BAG = 8,
    PAL_GOLD = 9,
    PAL_ALERT = 10,
    PAL_OK = 11,
    PAL_SHELL = 12
};

constexpr int kFlares = 5;
constexpr float FLARE_X[kFlares] = {36.f, 98.f, 160.f, 222.f, 284.f};
constexpr float FLARE_Y = 168.f;

struct Art {
    gs::Mipped parapet;
    gs::Mipped duck;
    gs::Mipped brazier;
    gs::Mipped flame[2];
    gs::Mipped sentry[2];
    gs::Mipped moon;
    gs::Mipped sun;
    gs::Mipped star;
    gs::Mipped burst;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace trench
