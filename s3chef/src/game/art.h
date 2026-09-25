// S3 CHEF sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace chef {

constexpr int TICKETS = 10;

enum Pal {
    PAL_HUD = 0,
    PAL_GOLD = 1,
    PAL_RED = 2,
    PAL_GREEN = 3,
    PAL_CHEF = 4,
    PAL_FOOD = 5,
    PAL_STEEL = 6,
    PAL_FIRE = 7,
    PAL_PAPER = 8,
    PAL_READY = 9,
    PAL_TRACK = 10,
    PAL_ZONE = 11,
    PAL_RAW = 12,
    PAL_OK = 13,
    PAL_HOT = 14,
    PAL_DECOR = 15
};

enum Word { WORD_TITLE = 0, WORD_SUB = 1, WORD_WIN = 2, WORD_BURN = 3, WORD_COUNT = 4 };

struct Art {
    int font[96] = {};
    gs::Mipped word[WORD_COUNT];
    gs::Mipped chef[2];
    gs::Mipped dish[TICKETS];
    gs::Mipped burnt;
    gs::Mipped pan;
    gs::Mipped flame[3];
    gs::Mipped ticket;
    gs::Mipped bell;
    gs::Mipped bracket;
    gs::Mipped lamp;
    gs::Mipped bottle;
    gs::Mipped shaker;
    gs::Mipped steam;
    gs::Mipped solid;
    gs::Mipped shade;
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace chef
