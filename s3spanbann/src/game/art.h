// S3 SPAN BANN sprites. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace sbann {

enum Pal {
    PAL_HUD = 0,
    PAL_WOOD = 1,
    PAL_STONE = 2,
    PAL_YOU = 3,
    PAL_FOE = 4,
    PAL_BANNER = 5,
    PAL_IRON = 6,
    PAL_FX = 7,
    PAL_ROCK = 8,
    PAL_AMBER = 9,
    PAL_ALERT = 10,
    PAL_GOOD = 11,
    PAL_TITLE = 12
};

struct Art {
    gs::Mipped runner[2];
    gs::Mipped air;
    gs::Mipped guard[2];
    gs::Mipped banner[2];
    gs::Mipped post;
    gs::Mipped plank;
    gs::Mipped lip;
    gs::Mipped link;
    gs::Mipped hook;
    gs::Mipped tower;
    gs::Mipped rock;
    gs::Mipped lamp;
    gs::Mipped moon;
    gs::Mipped cloud;
    gs::Mipped bat[2];
    gs::Mipped star;
    gs::Mipped splash;
    gs::Mipped chev;
    gs::Mipped wind;
    gs::Mipped dust;
    gs::Mipped word[4];  // span, back, over, wait
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace sbann
