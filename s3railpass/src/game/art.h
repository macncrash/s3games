// S3 RAIL PASS sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace railpass {

enum Pal {
    PAL_HUD = 0,
    PAL_ENGINE = 1,
    PAL_SNOW = 2,
    PAL_ROCK = 3,
    PAL_STORM = 4,
    PAL_RAIL = 5,
    PAL_PINE = 6,
    PAL_ALERT = 7
};

struct Art {
    gs::Mipped engine;
    gs::Mipped plow;
    gs::Mipped drift;
    gs::Mipped peak;
    gs::Mipped pine;
    gs::Mipped sleeper;
    gs::Mipped cloud;
    gs::Mipped mouth;
    gs::Mipped glyph[96];
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace railpass
