// S3 SKATECHIME pictures. Drawn at boot. No asset files.
#pragma once

#include "console/vdp.h"

namespace skatechime {

enum Pal {
    PAL_RIDER = 0,
    PAL_DECK = 1,
    PAL_GOLD = 2,
    PAL_STREET = 3,
    PAL_TOWER = 4,
    PAL_INK = 5,
    PAL_HOUR = 6,
    PAL_ALERT = 7,
    PAL_WORD = 8
};

struct Art {
    gs::Image rider;
    gs::Image deck;
    gs::Image brick;
    gs::Image gold;
    gs::Image tower;
    gs::Image face;
    gs::Image bell;
    gs::Image cone;
    gs::Image word;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace skatechime
