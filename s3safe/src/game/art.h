// S3 SAFE pictures. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace vault {

enum Pal {
    PAL_TEXT = 0,
    PAL_AMBER = 1,
    PAL_RED = 2,
    PAL_GREEN = 3,
    PAL_ROOM = 4,
    PAL_INK = 5,
    PAL_WHEEL = 6,
    PAL_DOOR = 7,
    PAL_RING = 8,
    PAL_PICK = 9,
    PAL_SHADE = 10
};

struct Art {
    gs::Mipped digit[10];
    gs::Mipped wheel[10];
    gs::Mipped door;
    gs::Mipped caret;
    gs::Mipped solid;
    int font[96] = {};
    float clueX[3] = {};
    float clueY[3] = {};
    float doorX = 0, doorY = 0, doorW = 0, doorH = 0;
    float dialX[3] = {};
    float dialY = 0;
    float slideOpen = 0;
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace vault
