// S3 CHEF SEVEN sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace chefseven {

constexpr int GOAL = 7;
constexpr int PANS = 3;

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
    PAL_RIVAL = 9,
    PAL_TRACK = 10,
    PAL_ZONE = 11,
    PAL_RAW = 12,
    PAL_OK = 13,
    PAL_HOT = 14,
    PAL_DECOR = 15
};

struct Art {
    int font[96] = {};
    gs::Mipped title;
    gs::Mipped sub;
    gs::Mipped win;
    gs::Mipped lose;
    gs::Mipped chef[2];
    gs::Mipped rival;
    gs::Mipped dish[4];
    gs::Mipped burnt;
    gs::Mipped pan;
    gs::Mipped flame[3];
    gs::Mipped bell;
    gs::Mipped solid;
    gs::Mipped shade;
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace chefseven
