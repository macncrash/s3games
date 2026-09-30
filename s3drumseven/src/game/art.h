// S3 DRUM SEVEN pictures. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace drumseven {

enum Pal {
    PAL_TEXT = 0,
    PAL_AMBER = 1,
    PAL_RED = 2,
    PAL_GREEN = 3,
    PAL_STAGE = 4,
    PAL_DRUM = 5,
    PAL_GOLD = 6,
    PAL_BLUE = 7,
    PAL_MALLET = 8,
    PAL_GLOW = 9
};

struct Art {
    gs::Mipped drum;
    gs::Mipped mallet;
    gs::Mipped glow;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace drumseven
