// S3 LOTBANN sprites. Drawn into VDP ROM at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace lotbann {

enum Pal {
    PAL_HUD = 0,
    PAL_MARK = 1,
    PAL_CAR = 2,
    PAL_CARB = 3,
    PAL_CARC = 4,
    PAL_PLAYER = 5,
    PAL_BANNER = 6,
    PAL_BOOTH = 7,
    PAL_LAMP = 8
};

struct Art {
    gs::Mipped runner;
    gs::Mipped car;
    gs::Mipped banner;
    gs::Mipped booth;
    gs::Mipped lamp;
    gs::Mipped dash;
    gs::Mipped pad;
    gs::Mipped puff;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace lotbann
