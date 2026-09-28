// S3 SCULL BOOM sprites. Drawn into sprite ROM at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace scullboom {

// Meters. +x is downstream, +y is up. The bed between the posts is the only
// place the drive counts as delivered.
constexpr double kDriveHalf = 1.55;
constexpr double kDriveH = 1.05;
constexpr double kDeck = 1.28;
constexpr double kBed0 = 148.4;
constexpr double kBed1 = 166.2;
constexpr double kPostL = 146.2;
constexpr double kPostR = 168.4;
constexpr double kPostW = 1.35;
constexpr double kPostTop = 4.6;
constexpr double kBow = 3.55;
constexpr double kStern = 4.15;
constexpr double kDriveSeat = 2.05;
constexpr double kHullY = 0.40;
constexpr double kFarBank = 196.0;

enum Pal {
    PAL_HUD = 0,
    PAL_AMBER = 1,
    PAL_BAD = 2,
    PAL_GOOD = 3,
    PAL_SHELL = 4,
    PAL_DRIVE = 5,
    PAL_BOOM = 6,
    PAL_WATER = 7,
    PAL_BANK = 8,
    PAL_TREE = 9,
    PAL_SIGN = 10,
    PAL_OAR = 11
};

struct Spr {
    gs::Mipped img;
    float ax = 0, ay = 0, ppm = 1;
};

struct Art {
    Spr shell;
    Spr rower;
    Spr oar[2];
    Spr drive;
    Spr post;
    Spr reed;
    Spr willow;
    Spr sign;
    Spr chev;
    Spr splash;
    gs::Mipped plank;
    gs::Mipped water[2];
    gs::Mipped grass;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace scullboom
