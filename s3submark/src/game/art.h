// S3 SUB MARK sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace submark {

enum Pal {
    PAL_HUD = 0,
    PAL_SUB = 1,
    PAL_MARK = 2,
    PAL_END = 3,
    PAL_BED = 4,
    PAL_KELP = 5,
    PAL_BUB = 6,
    PAL_WIN = 7,
    PAL_ALERT = 8,
    PAL_FISH = 9
};

struct Art {
    gs::Mipped sub;
    gs::Mipped pad;
    gs::Mipped cross;
    gs::Mipped pylon;
    gs::Mipped sand;
    gs::Mipped kelp;
    gs::Mipped bub;
    gs::Mipped fish;
    gs::Mipped lamp;
    gs::Mipped title;
    gs::Mipped set;
    gs::Mipped missed;
    gs::Mipped off;
    gs::Mipped ran;
    gs::Mipped hard;
    gs::Mipped paused;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace submark
