// S3 PAWNBELL pictures. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace pawnbell {

enum Pal {
    PAL_INK = 0,
    PAL_GOLD = 1,
    PAL_ALERT = 2,
    PAL_PAWN = 3,
    PAL_BELL = 4,
    PAL_BOB = 5,
    PAL_WOOD = 6,
    PAL_IVORY = 7
};

struct Art {
    gs::Image pawn;
    gs::Image bell;
    gs::Image bob;
    gs::Image crown;
    int font[96] = {};
    int tileLight = 1;
    int tileDark = 2;
    int tileWall = 3;
    int tileTrim = 4;
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace pawnbell
