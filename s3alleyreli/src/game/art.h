// S3 ALLEY RELIEF sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace alley {

enum Pal {
    PAL_HUD = 0,
    PAL_BRICK = 1,
    PAL_WATCH = 2,
    PAL_HOOD = 3,
    PAL_WOOD = 4,
    PAL_NEON = 5,
    PAL_BELL = 6,
    PAL_NIGHT = 7,
    PAL_FIELD = 12
};

struct Art {
    gs::Mipped wall;
    gs::Mipped escape;
    gs::Mipped dumpster;
    gs::Mipped watch[2];
    gs::Mipped hood[2];
    gs::Mipped bruiser;
    gs::Mipped barrel;
    gs::Mipped stick;
    gs::Mipped lantern;
    gs::Mipped lamp[2];
    gs::Mipped pole;
    gs::Mipped bell;
    gs::Mipped cord;
    gs::Mipped sign;
    gs::Mipped steam;
    gs::Mipped shadow;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace alley
