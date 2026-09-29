// S3 BOARD SEVEN pictures. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace boardseven {

enum Pal {
    PAL_TEXT = 0,
    PAL_AMBER = 1,
    PAL_RED = 2,
    PAL_GREEN = 3,
    PAL_BOARD = 4,
    PAL_BRASS = 5,
    PAL_LAMP = 6,
    PAL_MARK = 7,
    PAL_CORD = 8,
    PAL_IVORY = 9
};

constexpr int JACKS = 4;

struct Art {
    gs::Mipped jack;
    gs::Mipped lamp;
    gs::Mipped mark[JACKS];
    gs::Mipped plug;
    gs::Mipped solid;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace boardseven
