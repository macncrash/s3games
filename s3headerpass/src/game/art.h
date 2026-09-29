// S3 HEADER PASS pictures. Drawn into sprite ROM at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace headerpass {

enum Pal {
    PAL_INK = 0,
    PAL_GOLD = 1,
    PAL_BAD = 2,
    PAL_GOOD = 3,
    PAL_HULL = 4,
    PAL_SAIL = 5,
    PAL_ROCK = 6,
    PAL_CREST = 7,
    PAL_WATER = 12
};

struct Art {
    gs::Image glyph[96];
    gs::Mipped hull;
    gs::Mipped sail;
    gs::Mipped rock;
    gs::Mipped spire;
    gs::Mipped crest;
    gs::Mipped cloud;
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace headerpass
