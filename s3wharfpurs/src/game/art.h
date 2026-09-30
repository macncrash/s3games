// S3 WHARF PURSUIT sprites. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace wharfpurs {

enum Pal {
    PAL_INK = 0,
    PAL_GOLD = 1,
    PAL_ALERT = 2,
    PAL_GOOD = 3,
    PAL_YOU = 4,
    PAL_WINCH = 5,
    PAL_DOLLY = 6,
    PAL_STACK = 7,
    PAL_WOOD = 8,
    PAL_FX = 9,
    PAL_DUSK = 10,
    PAL_BOLT = 11,
    PAL_ROAD = 12
};

struct Art {
    gs::Mipped you;
    gs::Mipped winch;
    gs::Mipped dolly;
    gs::Mipped stacker;
    gs::Mipped piling;
    gs::Mipped lantern;
    gs::Mipped crane;
    gs::Mipped shed;
    gs::Mipped gull;
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

}  // namespace wharfpurs
