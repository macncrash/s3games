// S3 SAFEBELL pictures. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace safebell {

enum Pal {
    PAL_TEXT = 0,
    PAL_AMBER = 1,
    PAL_RED = 2,
    PAL_GREEN = 3,
    PAL_ROOM = 4,
    PAL_INK = 5,
    PAL_WHEEL = 6,
    PAL_DOOR = 7,
    PAL_BELL = 8,
    PAL_SHADE = 9,
    PAL_LAMP = 10
};

struct Art {
    gs::Mipped digit[10];
    gs::Mipped wheel[10];
    gs::Mipped door;
    gs::Mipped bell;
    gs::Mipped caret;
    gs::Mipped lamp;
    gs::Mipped solid;
    int font[96] = {};
    float clueX[3] = {};
    float clueY[3] = {};
    float doorX = 0, doorY = 0, doorW = 0, doorH = 0;
    float dialX[3] = {};
    float dialY = 0;
    float bellX = 0, bellY = 0;
    float slideOpen = 0;
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace safebell
