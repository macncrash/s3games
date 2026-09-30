// Tower art for S3 BELL BELL. Drawn into the VDP at boot.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace bellbell {

enum Pal {
    PAL_TOWER = 0,
    PAL_BELL = 1,
    PAL_MARK = 2,
    PAL_MALLET = 3,
    PAL_INK = 4,
    PAL_TITLE = 5,
    PAL_WIN = 6,
    PAL_DEAD = 7,
    PAL_HINT = 8
};

constexpr int BELLS = 3;

struct Art {
    gs::Image beam;
    gs::Image bell;
    gs::Image mark;
    gs::Image mallet;
    gs::Image stone;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace bellbell
