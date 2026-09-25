// S3 RAID sprites and yard tiles. Everything is drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace raid {

enum Pal : int {
    PAL_TEXT = 0,
    PAL_AMBER = 1,
    PAL_ALERT = 2,
    PAL_GOOD = 3,
    PAL_BIKE = 4,
    PAL_WOOD = 5,
    PAL_TREE = 6,
    PAL_STEEL = 7,
    PAL_FIRE = 8,
    PAL_MARK = 9,
    PAL_HILL = 10,
    PAL_LOGO = 11,
    PAL_ROAD = 12,
    PAL_YARD = 13,
    PAL_FLOOR = 14,
};

// Top-down yard. Screen center is the origin; +y is down the screen.
constexpr float kYardScale = 18.f;
constexpr float kYardCx = 160.f;
constexpr float kYardCy = 112.f;
constexpr float kYardLimX = 7.15f;
constexpr float kYardLimY = 4.95f;
constexpr float kYardInX = 6.55f;
constexpr float kYardInY = 4.45f;

struct Art {
    gs::Mipped bike[3];
    gs::Mipped top[8];
    gs::Mipped board;
    gs::Mipped tree;
    gs::Mipped post;
    gs::Mipped arch;
    gs::Mipped shack;
    gs::Mipped silo;
    gs::Mipped crate;
    gs::Mipped drum;
    gs::Mipped mast[2];
    gs::Mipped truck;
    gs::Mipped bullet;
    gs::Mipped puff;
    gs::Mipped shadow;
    gs::Mipped lamp;
    gs::Mipped sun;
    gs::Mipped stars;
    gs::Mipped logo;
    int font[96] = {};
    int dirt = 1;
    int dirtB = 1;
    int fence = 1;
    int lane = 1;
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace raid
