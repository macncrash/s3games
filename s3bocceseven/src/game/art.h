// S3 BOCCE SEVEN sprites. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace bocceseven {

enum Pal {
    PAL_INK = 0,
    PAL_CREAM = 1,
    PAL_YOU = 2,
    PAL_THEM = 3,
    PAL_PALLINO = 4,
    PAL_RAIL = 5,
    PAL_DUST = 6,
    PAL_CYPRESS = 7,
    PAL_LAMP = 8,
    PAL_MARK = 9,
    PAL_LAWN = 10,
    PAL_FLAG = 11
};

struct Art {
    gs::Mipped bowl;
    gs::Mipped pallino;
    gs::Mipped shade;
    gs::Mipped cypress;
    gs::Mipped lamp;
    gs::Mipped flag;
    gs::Mipped mark;
    int dust = 1;
    int rail = 1;
    int lawn = 1;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace bocceseven
