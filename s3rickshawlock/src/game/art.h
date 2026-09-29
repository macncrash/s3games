// S3 RICKSHAW LOCK sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace ricklock {

enum Pal : int {
    PAL_HUD = 0,
    PAL_CAB = 1,
    PAL_WHEEL = 2,
    PAL_GATE = 3,
    PAL_STONE = 4,
    PAL_WOOD = 5,
    PAL_BANK = 6,
    PAL_HOUSE = 7,
    PAL_WIN = 8,
    PAL_ALERT = 9,
    PAL_GOLD = 10,
    PAL_LAMP = 11,
};

struct Art {
    gs::Mipped body;
    gs::Mipped hood;
    gs::Mipped wheel;
    gs::Mipped driver;
    gs::Mipped seat;
    gs::Mipped gate;
    gs::Mipped stone;
    gs::Mipped plank;
    gs::Mipped grass;
    gs::Mipped house;
    gs::Mipped lamp;
    gs::Mipped beam;
    gs::Mipped title;
    gs::Mipped clear;
    gs::Mipped fail;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace ricklock
