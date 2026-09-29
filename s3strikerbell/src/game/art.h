// S3 STRIKER pictures. Drawn at boot. No asset files.
#pragma once

#include "console/vdp.h"

namespace striker {

enum Pal {
    PAL_FAIR = 0,
    PAL_TOWER = 1,
    PAL_BELL = 2,
    PAL_PUCK = 3,
    PAL_MALLET = 4,
    PAL_INK = 5,
    PAL_GOLD = 6,
    PAL_METER = 7,
    PAL_WIN = 8
};

struct Art {
    gs::Image tower;
    gs::Image bell;
    gs::Image puck;
    gs::Image mallet;
    gs::Image malletUp;
    gs::Image lamp;
    gs::Image base;
    gs::Image title;
    gs::Image sub;
    gs::Image hint;
    gs::Image swing;
    gs::Image miss;
    gs::Image ding;
    gs::Image dead;
    gs::Image digit[10];
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace striker
