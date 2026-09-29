// S3 SUB GRASS sprites. Drawn into VRAM at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace subgrass {

enum Pal {
    PAL_HUD = 0,
    PAL_TITLE = 1,
    PAL_WIN = 2,
    PAL_FAIL = 3,
    PAL_SUB = 4,
    PAL_BUB = 5,
    PAL_GRASS = 6,
    PAL_MUD = 7,
    PAL_ROCK = 8,
    PAL_KELP = 9
};

struct Art {
    gs::Mipped sub[3];
    gs::Mipped tuft;
    gs::Mipped ground;
    gs::Mipped bubble;
    gs::Mipped sailflag;
    gs::Mipped glyph[96];
    gs::Mipped title;
    gs::Mipped wordStop;
    gs::Mipped wordGrass;
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace subgrass
