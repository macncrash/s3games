// S3 HEADER BOOM pictures. Drawn into sprite ROM at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace headerboom {

enum Pal {
    PAL_INK = 0,
    PAL_GOLD = 1,
    PAL_BAD = 2,
    PAL_GOOD = 3,
    PAL_HULL = 4,
    PAL_DRIVE = 5,
    PAL_BOOM = 6,
    PAL_BANK = 7,
    PAL_MILL = 8,
    PAL_SKY = 9
};

struct Art {
    gs::Image glyph[96];
    gs::Mipped hull;
    gs::Mipped cabin;
    gs::Mipped sail;
    gs::Mipped drive;
    gs::Mipped plank;
    gs::Mipped post;
    gs::Mipped reed;
    gs::Mipped willow;
    gs::Mipped mill;
    gs::Mipped cloud;
    gs::Mipped bird;
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace headerboom
