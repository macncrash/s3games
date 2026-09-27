// S3 SPAN PURSUIT sprites. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace spanpurs {

enum Pal {
    PAL_HUD = 0,
    PAL_IRON = 1,
    PAL_JACK = 2,
    PAL_DRUM = 3,
    PAL_CRAWL = 4,
    PAL_WAGON = 5,
    PAL_DRAY = 6,
    PAL_ROCK = 7,
    PAL_ROPE = 8,
    PAL_FX = 9,
    PAL_DEAD = 10,
    PAL_AMBER = 11,
    PAL_ALERT = 12,
    PAL_GOOD = 13,
    PAL_TITLE = 14,
    PAL_MIST = 15
};

struct Art {
    gs::Mipped jack[2];
    gs::Mipped drum[2];
    gs::Mipped crawl[2];
    gs::Mipped wagon[2];
    gs::Mipped dray[2];
    gs::Mipped plank;
    gs::Mipped truss;
    gs::Mipped rope;
    gs::Mipped link;
    gs::Mipped tower;
    gs::Mipped cliff;
    gs::Mipped lip;
    gs::Mipped cloud;
    gs::Mipped sun;
    gs::Mipped bird;
    gs::Mipped flame[2];
    gs::Mipped dust;
    gs::Mipped chev;
    gs::Mipped pennant;
    gs::Mipped title;
    gs::Mipped last;
    gs::Mipped seized;
    gs::Mipped paused;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace spanpurs
