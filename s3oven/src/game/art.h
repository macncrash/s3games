// S3 OVEN sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace oven {

enum Pal {
    PAL_INK = 0,
    PAL_GOLD = 1,
    PAL_ALERT = 2,
    PAL_OK = 3,
    PAL_LOAF = 4,
    PAL_BRICK = 5,
    PAL_FIRE = 6,
    PAL_WOOD = 7,
    PAL_TRACK = 8,
    PAL_MARK = 9,
    PAL_DAWN = 10,
    PAL_CLOTH = 11,
    PAL_ASH = 12,
    PAL_FLOUR = 13,
    PAL_SELECT = 14,
    PAL_SOOT = 15
};

enum LoafLook {
    LOOK_RAW = 0,
    LOOK_PALE,
    LOOK_TURN,
    LOOK_DANGER,
    LOOK_FLIP,
    LOOK_BAKE,
    LOOK_DRAW,
    LOOK_DARK,
    LOOK_CHAR,
    LOOK_COUNT
};

struct Art {
    int font[96] = {};
    gs::Mipped loaf[LOOK_COUNT];
    gs::Mipped crumb;
    gs::Mipped arch;
    gs::Mipped flame[3];
    gs::Mipped peel;
    gs::Mipped board;
    gs::Mipped steam;
    gs::Mipped spark;
    gs::Mipped solid;
    gs::Mipped sun;
    gs::Mipped title;
    gs::Mipped sub;
    gs::Mipped winWord;
    gs::Mipped failWord;
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace oven
