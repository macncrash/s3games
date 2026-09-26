// S3 RIDGE PURSUIT sprites. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace ridgepurs {

enum Pal {
    PAL_INK = 0,
    PAL_GOLD = 1,
    PAL_ALERT = 2,
    PAL_GOOD = 3,
    PAL_YOU = 4,
    PAL_CRAWL = 5,
    PAL_DRAY = 6,
    PAL_HAUL = 7,
    PAL_STONE = 8,
    PAL_FX = 9,
    PAL_PEAK = 10,
    PAL_BOLT = 11,
    PAL_ROAD = 12
};

struct Art {
    gs::Mipped you;
    gs::Mipped crawler;
    gs::Mipped dray;
    gs::Mipped hauler;
    gs::Mipped cairn;
    gs::Mipped post;
    gs::Mipped peakL;
    gs::Mipped peakR;
    gs::Mipped cloud;
    gs::Mipped sun;
    gs::Mipped moon;
    gs::Mipped shot;
    gs::Mipped bolt;
    gs::Mipped puff;
    gs::Mipped spark;
    gs::Mipped flame[2];
    gs::Mipped shadow;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace ridgepurs
