// S3 CLIFFSLIP pictures. Drawn into VDP memory at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace slip {

enum Pal {
    PAL_TEXT = 0,
    PAL_WARN = 1,
    PAL_GOOD = 2,
    PAL_DIM = 3,
    PAL_MAP = 4,
    PAL_BOAT = 5,
    PAL_WAKE = 6
};

enum MapInk {
    C_DEEP = 1,
    C_MID = 2,
    C_SHALLOW = 3,
    C_FOAM = 4,
    C_ROCK = 5,
    C_STONE = 6,
    C_LIT = 7,
    C_WOOD = 8,
    C_ROPE = 9,
    C_MOSS = 10,
    C_SAND = 11,
    C_BUOY = 12,
    C_LAMP = 13,
    C_SHADOW = 14,
    C_MARK = 15
};

constexpr int BOAT_FRAMES = 16;
constexpr int BOAT_PX = 32;

struct Art {
    gs::Image glyph[96];
    int gw[96] = {};
    int gh = 8;
    gs::Image boat[BOAT_FRAMES];
    gs::Image wake;
    gs::Image gull;
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace slip
