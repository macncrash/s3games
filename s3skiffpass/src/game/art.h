// S3 SKIFF PASS sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace skiffpass {

enum Pal : int {
    PAL_HUD = 0,
    PAL_AMBER = 1,
    PAL_RED = 2,
    PAL_GREEN = 3,
    PAL_BOAT = 4,
    PAL_CLIFF = 5,
    PAL_ROCK = 6,
    PAL_PINE = 7,
    PAL_FX = 8,
    PAL_BIRD = 9,
    PAL_STORM = 10,
    PAL_HUT = 11,
    PAL_SIGN = 12,
    PAL_BUOY = 13
};

constexpr int YAWS = 16;
constexpr float HULL_PX = 60.f;

struct Art {
    gs::Mipped skiff[YAWS];
    gs::Mipped cliff;
    gs::Mipped pine;
    gs::Mipped skerry;
    gs::Mipped buoy;
    gs::Mipped beacon;
    gs::Mipped hut;
    gs::Mipped sign;
    gs::Mipped gull[2];
    gs::Mipped ripple;
    gs::Mipped foam;
    gs::Mipped shadow;
    gs::Mipped cloud;
    gs::Mipped rain;
    gs::Mipped bolt;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace skiffpass
