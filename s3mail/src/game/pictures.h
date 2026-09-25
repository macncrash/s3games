// S3 MAIL pictures. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace mail {

enum Pal {
    PAL_HUD = 0,
    PAL_GOLD = 1,
    PAL_RED = 2,
    PAL_GREEN = 3,
    PAL_BIKE = 4,
    PAL_HOUSE = 5,
    PAL_TREE = 6,
    PAL_BOX = 7,
    PAL_SIGN = 8,
    PAL_PAPER = 9,
    PAL_FX = 10,
    PAL_ROAD = 12
};

struct Art {
    gs::Mipped bike[3];
    gs::Mipped boxShut, boxOpen;
    gs::Mipped house[3];
    gs::Mipped tree;
    gs::Mipped lamp;
    gs::Mipped sign;
    gs::Mipped depot;
    gs::Mipped paper;
    gs::Mipped puff;
    gs::Mipped cloud;
    gs::Mipped sun;
    gs::Mipped shadow;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace mail
