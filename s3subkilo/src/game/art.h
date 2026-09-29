// S3 SUB KILO sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace subkilo {

enum Pal {
    PAL_HUD = 0,
    PAL_SUB = 1,
    PAL_WHEEL = 2,
    PAL_GATE = 3,
    PAL_FX = 4,
    PAL_KELP = 5
};

struct Art {
    gs::Mipped sub;
    gs::Mipped wheel[4];
    gs::Mipped post;
    gs::Mipped bubble;
    gs::Mipped kelp;
    gs::Mipped glyph[96];
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace subkilo
