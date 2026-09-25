// Pictures drawn at boot. Nothing is loaded from a file.
#pragma once

#include "console/gfx.h"
#include "console/vdp.h"

namespace siege {

enum Pal {
    PAL_HUD = 0,
    PAL_GOLD = 1,
    PAL_ALERT = 2,
    PAL_DIM = 3,
    PAL_WALL = 4,
    PAL_GATE = 5,
    PAL_RAM = 6,
    PAL_IRON = 7,
    PAL_ROCK = 8,
    PAL_DUST = 9,
    PAL_BANNER = 10,
    PAL_FIRE = 11,
    PAL_GRASS = 12,
    PAL_MOON = 13
};

struct Art {
    int font[96] = {};
    gs::Mipped title, sub, hold, broke, through, pause;
    gs::Mipped you, youBrace;
    gs::Mipped gate[3];
    gs::Mipped ram[2], iron[2];
    gs::Mipped rock, puff, shadow;
    gs::Mipped merlon, course, tower, banner;
    gs::Mipped torch[2];
    gs::Mipped moon, star, tuft;
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace siege
