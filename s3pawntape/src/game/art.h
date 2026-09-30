// S3 PAWNTAPE pictures. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace pawntape {

enum Pal {
    PAL_TEXT = 0,
    PAL_AMBER = 1,
    PAL_RED = 2,
    PAL_GREEN = 3,
    PAL_FELT = 4,
    PAL_IVORY = 5,
    PAL_EBONY = 6,
    PAL_GOLD = 7,
    PAL_GHOST = 8,
    PAL_PAPER = 9
};

struct Art {
    gs::Mipped ivory;
    gs::Mipped ebony;
    gs::Mipped crown;
    gs::Mipped ghost;
    gs::Mipped slip;
    gs::Mipped solid;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace pawntape
