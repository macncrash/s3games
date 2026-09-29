// S3 SUB BOOM sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace subboom {

enum Pal : int {
    PAL_HUD = 0,
    PAL_SUB = 1,
    PAL_DRIVE = 2,
    PAL_BOOM = 3,
    PAL_ROCK = 4,
    PAL_BUB = 5,
    PAL_FISH = 6,
    PAL_WIN = 7,
    PAL_ALERT = 8,
    PAL_LAMP = 9
};

struct Art {
    gs::Mipped sub;
    gs::Mipped drive;
    gs::Mipped boom;
    gs::Mipped pile;
    gs::Mipped rock;
    gs::Mipped kelp;
    gs::Mipped bub;
    gs::Mipped fish;
    gs::Mipped lamp;
    gs::Mipped title;
    gs::Mipped delivered;
    gs::Mipped missed;
    gs::Mipped offLeg;
    gs::Mipped out;
    gs::Mipped paused;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace subboom
