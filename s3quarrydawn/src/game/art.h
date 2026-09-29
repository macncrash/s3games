// S3 QUARRY DAWN sprites. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace qdawn {

enum Pal {
    PAL_HUD = 0,
    PAL_ROCK = 1,
    PAL_CUT = 2,
    PAL_YOU = 3,
    PAL_POT = 4,
    PAL_FIRE = 5,
    PAL_MOON = 6,
    PAL_STAR = 7,
    PAL_CRANE = 8,
    PAL_GOLD = 9,
    PAL_ALERT = 10,
    PAL_OK = 11,
    PAL_WATER = 12
};

constexpr int kFlares = 4;
constexpr int kTiers = 3;
constexpr float LADDER_X = 158.f;
// Screen y of the worker's feet on each bench, high rim to pit floor.
constexpr float TIER_Y[kTiers] = {112.f, 152.f, 190.f};

struct FlareSpot {
    float x;
    int tier;
};

constexpr FlareSpot kSpot[kFlares] = {
    {58.f, 0},
    {262.f, 0},
    {236.f, 1},
    {96.f, 2},
};

struct Art {
    gs::Mipped face;
    gs::Mipped water;
    gs::Mipped crane;
    gs::Mipped pot;
    gs::Mipped flame[2];
    gs::Mipped man[2];
    gs::Mipped moon;
    gs::Mipped sun;
    gs::Mipped star;
    gs::Mipped rung;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace qdawn
