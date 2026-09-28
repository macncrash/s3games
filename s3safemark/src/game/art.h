// S3 SAFEMARK pictures. Drawn at boot. No asset files.
#pragma once

#include "console/gfx.h"
#include "console/vdp.h"

namespace safemark {

enum Pal {
    PAL_TEXT = 0,
    PAL_AMBER = 1,
    PAL_RED = 2,
    PAL_GREEN = 3,
    PAL_ROOM = 4,
    PAL_INK = 5,
    PAL_WHEEL = 6,
    PAL_DOOR = 7,
    PAL_GOLD = 8,
    PAL_SHADE = 9
};

struct Art {
    gs::Mipped digit[10];
    gs::Mipped wheel;
    gs::Mipped door;
    gs::Mipped caret;
    gs::Mipped ring;
    gs::Mipped stamp;
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

}  // namespace safemark
