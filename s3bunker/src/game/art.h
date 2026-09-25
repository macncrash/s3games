// S3 BUNKER sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace bunker {

enum Pal {
    PAL_HUD = 0,
    PAL_ROOM = 1,
    PAL_DOOR = 2,
    PAL_AMBER = 3,
    PAL_YOU = 4,
    PAL_FOE = 5,
    PAL_BRUTE = 6,
    PAL_FX = 7,
    PAL_ALERT = 8,
    PAL_OK = 9
};

// The blast door sits in the far wall. The slit is a hole in that sprite.
constexpr int DOOR_X = 70;
constexpr int DOOR_Y = 18;
constexpr int DOOR_W = 180;
constexpr int DOOR_H = 186;
constexpr int SLIT_X = 88;
constexpr int SLIT_Y = 72;
constexpr int SLIT_W = 144;
constexpr int SLIT_H = 52;
constexpr int BAR_X = 102;
constexpr int BAR_Y = 136;
constexpr int BAR_W = 116;
constexpr int BAR_H = 12;

struct Art {
    gs::Mipped door, bar, crack;
    gs::Mipped walker[2], ducker[2], breacher[2];
    gs::Mipped rifle, sight, flash, spark, dust;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace bunker
