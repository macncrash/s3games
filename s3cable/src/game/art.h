// S3 CABLE sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace cable {

// The hill is one fixed diagonal on the 320x224 screen. A window of kView
// meters of track is laid along that line; the world slides, the line does not.
constexpr float kX0 = 16.f;
constexpr float kY0 = 202.f;
constexpr float kDx = 288.f;
constexpr float kDy = -168.f;
constexpr float kView = 22.f;

enum Pal {
    PAL_HUD = 0,
    PAL_CAR = 1,
    PAL_LAND = 2,
    PAL_HOUSE = 3,
    PAL_TREE = 4,
    PAL_MARK = 5,
    PAL_STEEL = 6,
    PAL_PEOPLE = 7,
    PAL_ROCK = 8,
    PAL_LAMP = 9,
    PAL_WATER = 10,
    PAL_BG = 11
};

struct Art {
    gs::Mipped car;
    float sillX = 0, sillY = 0;
    gs::Mipped sleeper;
    float sleepX = 0, sleepY = 0;
    gs::Mipped plat[3];
    float platX = 0, platY = 0;
    gs::Mipped house[3];
    gs::Mipped tree, rock, lamp, person[2], sheave, bumper, water, bird, bar, tick;
    gs::Image title, line1, line2, level, ran, enter;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace cable
