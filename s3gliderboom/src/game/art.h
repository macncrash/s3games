// S3 GLIDER BOOM sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace gboom {

// Shared world, meters. +x is the direction of flight, +y is up. The bed is the
// only place a delivery counts; the inset outside it is still the boom's edge.
constexpr double kDriveHalf = 4.15;
constexpr double kDriveThick = 3.05;
constexpr double kDeckY = 1.20;
constexpr double kBoomX0 = 112.0;
constexpr double kBoomX1 = 138.0;
constexpr double kBoomInset = 2.40;
constexpr double kBedX0 = kBoomX0 + kBoomInset;
constexpr double kBedX1 = kBoomX1 - kBoomInset;
constexpr double kStickW = 1.20;
constexpr double kPostTop = 4.80;
constexpr double kShoreX = 64.0;
constexpr double kFarShoreX = 146.0;

enum Pal {
    PAL_HUD = 0,
    PAL_AMBER = 1,
    PAL_BAD = 2,
    PAL_GOOD = 3,
    PAL_SHIP = 4,
    PAL_DRIVE = 5,
    PAL_BOOM = 6,
    PAL_WATER = 7,
    PAL_BANK = 8,
    PAL_MILL = 9,
    PAL_TREE = 10,
    PAL_SKY = 11,
    PAL_SIGN = 12,
    PAL_SPLASH = 13,
    PAL_BIRD = 14
};

struct Spr {
    gs::Mipped img;
    float ax = 0, ay = 0, ppm = 1;
};

struct Art {
    Spr ship[5];
    Spr drive;
    Spr stick;
    Spr log;
    Spr sign;
    Spr mill;
    Spr pine;
    Spr reed;
    Spr cloud;
    Spr sun;
    Spr hill;
    Spr chev;
    Spr splash;
    Spr shade;
    Spr bird[2];
    gs::Mipped plank;
    gs::Mipped water[2];
    gs::Mipped grass[2];
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace gboom
