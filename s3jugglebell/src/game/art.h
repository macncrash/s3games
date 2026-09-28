// S3 JUGGLEBELL pictures. Drawn at boot. No asset files.
#pragma once

#include "console/gfx.h"
#include "console/vdp.h"

namespace jugglebell {

enum Pal {
    PAL_INK = 1,
    PAL_GOLD = 2,
    PAL_GREEN = 3,
    PAL_ALERT = 4,
    PAL_BELL = 5,
    PAL_BALL = 6,
    PAL_GOLD_BALL = 7,
    PAL_BLUE = 8,
    PAL_BODY = 9,
    PAL_WOOD = 10,
    PAL_CLOTH = 11,
    PAL_TITLE = 12
};

struct Art {
    gs::Mipped bell;
    gs::Image clapper;
    gs::Image rope;
    gs::Image ball;
    gs::Image juggler;
    gs::Image hand;
    gs::Image post;
    gs::Image meter;
    gs::Image pip;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace jugglebell
