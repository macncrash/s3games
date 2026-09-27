// S3 SKATEBELL pictures. Drawn at boot. No asset files.
#pragma once

#include "console/vdp.h"

namespace skatebell {

enum Pal {
    PAL_RIDER = 0,
    PAL_DECK = 1,
    PAL_BELL = 2,
    PAL_STREET = 3,
    PAL_TOWN = 4,
    PAL_INK = 5,
    PAL_GOLD = 6,
    PAL_ALERT = 7,
    PAL_WORD = 8
};

struct Art {
    gs::Image rider;
    gs::Image deck;
    gs::Image bell;
    gs::Image brick;
    gs::Image pole;
    gs::Image cone;
    gs::Image block;
    gs::Image word;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace skatebell
