// S3 CHEFMARK pictures. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace chefmark {

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
    PAL_INK = 9
};

struct Art {
    int font[96] = {};
    gs::Mipped title;
    gs::Mipped done;
    gs::Mipped raw;
    gs::Mipped burn;
    gs::Mipped chef;
    gs::Mipped steak;
    gs::Mipped charred;
    gs::Mipped pan;
    gs::Mipped flame[3];
    gs::Mipped ticket;
    gs::Mipped mark;
    gs::Mipped solid;
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace chefmark
