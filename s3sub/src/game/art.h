// S3 SUB sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"

namespace sub {

enum Pal : int {
    PAL_HUD = 0,
    PAL_SUB = 1,
    PAL_HIT = 2,
    PAL_ROCK = 3,
    PAL_CEIL = 4,
    PAL_FISH = 5,
    PAL_LAMP = 6,
    PAL_GATE = 7,
    PAL_ALERT = 8,
    PAL_WARN = 9,
    PAL_WIN = 10,
    PAL_DIM = 11,
};

struct Art {
    gs::Mipped sub;
    gs::Mipped prop[2];
    gs::Mipped rock[3];
    gs::Mipped ceil[3];
    gs::Mipped fish[2];
    gs::Mipped mote;
    gs::Mipped puff;
    gs::Mipped ring;
    gs::Mipped bulb;
    gs::Mipped gate;
    gs::Mipped glow;
    gs::Mipped title;
    gs::Mipped tag;
    gs::Mipped clear;
    gs::Mipped breach;
    gs::Mipped paused;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace sub
