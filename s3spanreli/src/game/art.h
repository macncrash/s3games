// S3 SPAN RELIEF sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"

namespace spanreli {

enum Pal {
    PAL_HUD = 0,
    PAL_DECK = 1,
    PAL_COAT = 2,
    PAL_KEEPER = 3,
    PAL_STONE = 4,
    PAL_WAGON = 5,
    PAL_BELL = 6,
    PAL_NIGHT = 7,
    PAL_AMBER = 8,
    PAL_ALERT = 9,
    PAL_GOOD = 10,
    PAL_TITLE = 11,
    PAL_HEAT = 12,
    PAL_DIM = 13
};

struct Art {
    gs::Mipped plank;
    gs::Mipped hanger;
    gs::Mipped taut;
    gs::Mipped hot;
    gs::Mipped link;
    gs::Mipped yoke;
    gs::Mipped pier;
    gs::Mipped belfry;
    gs::Mipped cliff;
    gs::Mipped wall;
    gs::Mipped lip;
    gs::Mipped coat[2];
    gs::Mipped drum[2];
    gs::Mipped wagon[2];
    gs::Mipped keeper[2];
    gs::Mipped bell;
    gs::Mipped rope;
    gs::Mipped pennant;
    gs::Mipped flame[2];
    gs::Mipped chev;
    gs::Mipped streak;
    gs::Mipped dust;
    gs::Mipped moon;
    gs::Mipped cloud;
    gs::Mipped bird;
    gs::Mipped glint;
    gs::Mipped title;
    gs::Mipped held;
    gs::Mipped broke;
    gs::Mipped late;
    gs::Mipped paused;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace spanreli
