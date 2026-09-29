// S3 MAIL VAN GRASS sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace vangrass {

constexpr float kHalf = 7.4f;
constexpr float kGrassL = 136.f;
constexpr float kGrassR = 186.f;
constexpr float kWest = 6.f;
constexpr float kGoal = (kGrassL + kGrassR) * 0.5f;

enum Pal : int {
    PAL_HUD = 0,
    PAL_VAN = 1,
    PAL_GRASS = 2,
    PAL_ROAD = 3,
    PAL_POST = 4,
    PAL_HOUSE = 5,
    PAL_END = 6,
    PAL_SMOKE = 7,
    PAL_SKY = 8
};

struct Art {
    gs::Mipped van;
    gs::Mipped wheel;
    gs::Mipped sod;
    gs::Mipped tuft;
    gs::Mipped road;
    gs::Mipped dash;
    gs::Mipped house;
    gs::Mipped box;
    gs::Mipped lamp;
    gs::Mipped endpost;
    gs::Mipped drop;
    gs::Mipped smoke;
    int font[96] = {};
    int skyTile = 1;
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace vangrass
