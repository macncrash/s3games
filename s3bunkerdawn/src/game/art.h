// S3 BUNKER DAWN sprites. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace bdawn {

enum Pal {
    PAL_HUD = 0,
    PAL_YARD = 1,
    PAL_BLOCK = 2,
    PAL_YOU = 3,
    PAL_POT = 4,
    PAL_FIRE = 5,
    PAL_MOON = 6,
    PAL_STAR = 7,
    PAL_BAG = 8,
    PAL_GOLD = 9,
    PAL_ALERT = 10,
    PAL_OK = 11,
    PAL_WIND = 12
};

constexpr int kFlares = 4;
constexpr float FLARE_X[kFlares] = {54.f, 122.f, 198.f, 270.f};
constexpr float FLARE_Y = 164.f;
constexpr float GROUND_Y = 168.f;

struct Art {
    gs::Mipped bunker;
    gs::Mipped bag;
    gs::Mipped pot;
    gs::Mipped flame[2];
    gs::Mipped man[2];
    gs::Mipped moon;
    gs::Mipped sun;
    gs::Mipped star;
    gs::Mipped spark;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace bdawn
