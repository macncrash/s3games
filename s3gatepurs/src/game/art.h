// S3 GATE PURSUIT sprites. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace purs {

enum Pal {
    PAL_INK = 0,
    PAL_GOLD = 1,
    PAL_ALERT = 2,
    PAL_GOOD = 3,
    PAL_YOU = 4,
    PAL_SCOUT = 5,
    PAL_WAGON = 6,
    PAL_HEAVY = 7,
    PAL_STONE = 8,
    PAL_FX = 9,
    PAL_NIGHT = 10,
    PAL_SHOT = 11,
    PAL_ROAD = 12,
    PAL_BOLT = 13,
    PAL_TREE = 14
};

struct Art {
    gs::Mipped you;
    gs::Mipped scout;
    gs::Mipped wagon;
    gs::Mipped heavy;
    gs::Mipped tower;
    gs::Mipped grate;
    gs::Mipped beam;
    gs::Mipped banner;
    gs::Mipped flame[2];
    gs::Mipped lamp;
    gs::Mipped shot;
    gs::Mipped bolt;
    gs::Mipped puff;
    gs::Mipped spark;
    gs::Mipped shadow;
    gs::Mipped post;
    gs::Mipped tree;
    gs::Mipped moon;
    gs::Mipped star;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace purs
