// S3 SPAN sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"

namespace span {

enum Pal {
    PAL_HUD = 0,
    PAL_WOOD = 1,
    PAL_COAT = 2,
    PAL_JACK = 3,
    PAL_ROCK = 4,
    PAL_CART = 5,
    PAL_FX = 6,
    PAL_ROPE = 7,
    PAL_AMBER = 8,
    PAL_ALERT = 9,
    PAL_GOOD = 10,
    PAL_TITLE = 11
};

struct Art {
    gs::Mipped plank;
    gs::Mipped rope;
    gs::Mipped taut;
    gs::Mipped link;
    gs::Mipped tower;
    gs::Mipped cliff;
    gs::Mipped wall;
    gs::Mipped lip;
    gs::Mipped march[2];
    gs::Mipped banner[2];
    gs::Mipped cart[2];
    gs::Mipped keeper[2];
    gs::Mipped chev;
    gs::Mipped cloud;
    gs::Mipped flame[2];
    gs::Mipped dust;
    gs::Mipped glint;
    gs::Mipped streak;
    gs::Mipped pennant;
    gs::Mipped sun;
    gs::Mipped bird;
    gs::Mipped title;
    gs::Mipped across;
    gs::Mipped broke;
    gs::Mipped paused;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace span
