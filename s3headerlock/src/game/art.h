// S3 HEADER LOCK sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace headerlock {

enum Pal : int {
    PAL_HUD = 0,
    PAL_CAR = 1,
    PAL_GATE = 2,
    PAL_TREE = 3,
    PAL_BANNER = 4,
    PAL_SIGN = 5,
    PAL_ROAD = 12,
};

struct Art {
    gs::Mipped car;
    gs::Mipped post;
    gs::Mipped tree;
    gs::Mipped banner;
    gs::Mipped sign;
    gs::Mipped title;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace headerlock
