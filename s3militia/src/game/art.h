// S3 MILITIA sprites and yard tiles. Everything is drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace militia {

enum Pal {
    PAL_WHITE = 0,
    PAL_GOLD = 1,
    PAL_ALERT = 2,
    PAL_GOOD = 3,
    PAL_MIL = 4,
    PAL_RAID = 5,
    PAL_TORCH = 6,
    PAL_RUN = 7,
    PAL_WELL = 8,
    PAL_FX = 9,
    PAL_TREE = 10,
    PAL_GROUND = 11
};

struct Art {
    gs::Mipped mil[3][2];
    gs::Mipped run[3][2];
    gs::Mipped raid[3][2];
    gs::Mipped torch[3][2];
    gs::Mipped well;
    gs::Mipped crack;
    gs::Mipped glint;
    gs::Mipped tree;
    gs::Mipped bucket;
    gs::Mipped ball;
    gs::Mipped flash;
    gs::Mipped puff;
    gs::Mipped shadow;
    gs::Mipped glyph[96];
    int font[96] = {};
    int grass[3] = {};
    int dirt = 1;
    int cobble = 1;
    int fenceH = 1;
    int fenceV = 1;
};

void buildArt(gs::VDP& vdp, Art& art);
void yardTint(gs::VDP& vdp, int wave, uint16_t& sky);

}  // namespace militia
