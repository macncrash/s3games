// S3 BUNKER PURSUIT sprites. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace bunkerpurs {

enum Pal {
    PAL_HUD = 0,
    PAL_CONC = 1,
    PAL_PLANT = 2,
    PAL_LIGHT = 3,
    PAL_HEAVY = 4,
    PAL_BOLT = 5,
    PAL_FX = 6,
    PAL_DOOR = 7,
    PAL_WRECK = 8
};

struct Art {
    gs::Mipped plant;
    gs::Mipped lightM;
    gs::Mipped heavyM;
    gs::Mipped bolt;
    gs::Mipped ebolt;
    gs::Mipped spark;
    gs::Mipped door;
    gs::Mipped bay;
    gs::Mipped lamp;
    gs::Mipped titleA;
    gs::Mipped titleB;
    gs::Mipped sub;
    gs::Mipped glyph[96];
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace bunkerpurs
