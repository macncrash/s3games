// S3 JUGGLEMARK pictures. Drawn at boot. No asset files.
#pragma once

#include "console/gfx.h"
#include "console/vdp.h"

namespace jugglemark {

enum Pal {
    PAL_BODY = 0,
    PAL_GLOVE = 1,
    PAL_RED = 2,
    PAL_GOLD = 3,
    PAL_STAGE = 4,
    PAL_LAMP = 5,
    PAL_INK = 6,
    PAL_MARK = 7,
    PAL_ALERT = 8,
    PAL_TITLE = 9,
    PAL_WIN = 10
};

struct Art {
    gs::Image juggler;
    gs::Image glove;
    gs::Image ball;
    gs::Image gold;
    gs::Image stage;
    gs::Image lamp;
    gs::Image chalk;
    gs::Image title;
    gs::Image win;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace jugglemark
