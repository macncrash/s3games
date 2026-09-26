// S3 LOCK sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace s3lock {

enum Pal {
    PAL_HUD = 0,
    PAL_AMBER = 1,
    PAL_RED = 2,
    PAL_GREEN = 3,
    PAL_BOAT = 4,
    PAL_GATE = 5,
    PAL_STONE = 6,
    PAL_BANK = 7,
    PAL_FX = 8,
    PAL_KEEP = 9
};

constexpr int YAWS = 7;
constexpr float YAW_SPAN = 12.f;  // degrees either side of straight upstream
constexpr int BOAT_SRC_H = 104;
constexpr int GATE_DIRS = 72;  // one timber every 5 degrees
constexpr int LEAF_SRC_L = 64;

struct Art {
    gs::Mipped boat[YAWS];
    gs::Mipped leaf[GATE_DIRS];
    gs::Mipped stone;
    gs::Mipped grass;
    gs::Mipped field;
    gs::Mipped tree;
    gs::Mipped keeper;
    gs::Mipped buoy;
    gs::Mipped ripple;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace s3lock
