// S3 MAILVAN MARK sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace mailvanmark {

enum Pal : int {
    PAL_HUD = 0,
    PAL_VAN = 1,
    PAL_BANK = 2,
    PAL_POST = 3,
    PAL_MARK = 4,
    PAL_CREW = 5,
    PAL_CLOCK = 6,
    PAL_FOAM = 7,
    PAL_WIN = 8,
    PAL_ALERT = 9,
    PAL_BANNER = 10,
    PAL_SHED = 11,
    PAL_TREE = 12,
    PAL_BIRD = 13,
    PAL_WATER = 14,
};

struct Art {
    gs::Mipped van;
    gs::Mipped road;
    gs::Mipped wake;
    gs::Mipped smoke;
    gs::Mipped bank;
    gs::Mipped reed;
    gs::Mipped post;
    gs::Mipped cross;
    gs::Mipped ring;
    gs::Mipped shed;
    gs::Mipped clock[8];
    gs::Mipped crew[2];
    gs::Mipped tree;
    gs::Mipped lamp;
    gs::Mipped bird[2];
    gs::Mipped title;
    gs::Mipped wordMark;
    gs::Mipped setDown;
    gs::Mipped lost;
    gs::Mipped paused;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace mailvanmark
