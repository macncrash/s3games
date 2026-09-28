// S3 SAFETAPE pictures. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace safetape {

enum Pal {
    PAL_TEXT = 0,
    PAL_AMBER = 1,
    PAL_RED = 2,
    PAL_GREEN = 3,
    PAL_ROOM = 4,
    PAL_INK = 5,
    PAL_WHEEL = 6,
    PAL_DOOR = 7,
    PAL_TAPE = 8,
    PAL_WALK = 9,
    PAL_SHADE = 10
};

struct Art {
    gs::Mipped digit[10];
    gs::Mipped door;
    gs::Mipped caret;
    gs::Mipped solid;
    gs::Mipped walker;
    int font[96] = {};
    float tapeX[3] = {};
    float tapeY[3] = {};
    float nearX[3] = {};
    float nearY = 0;
    float dialX[3] = {};
    float dialY = 0;
    float drawerX[3] = {};
    float drawerY = 0;
    float doorX = 0, doorY = 0, doorW = 0, doorH = 0;
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace safetape
