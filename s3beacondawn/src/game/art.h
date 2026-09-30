// S3 BEACON DAWN sprites. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace bdawn {

enum Pal {
    PAL_HUD = 0,
    PAL_CLIFF = 1,
    PAL_TOWER = 2,
    PAL_YOU = 3,
    PAL_BOWL = 4,
    PAL_FIRE = 5,
    PAL_MOON = 6,
    PAL_STAR = 7,
    PAL_SEA = 8,
    PAL_GOLD = 9,
    PAL_ALERT = 10,
    PAL_OK = 11,
    PAL_RAIN = 12
};

constexpr int kFlares = 5;
constexpr float FLARE_X[kFlares] = {36.f, 96.f, 160.f, 224.f, 284.f};
constexpr float FLARE_Y = 158.f;
constexpr float GROUND_Y = 176.f;

struct Art {
    gs::Mipped tower;
    gs::Mipped bowl;
    gs::Mipped flame[2];
    gs::Mipped keeper[2];
    gs::Mipped lamp;
    gs::Mipped moon;
    gs::Mipped sun;
    gs::Mipped star;
    gs::Mipped drop;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace bdawn
