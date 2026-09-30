// S3 SALLY DOOR pictures. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace sally {

enum Pal {
    PAL_TEXT = 0,
    PAL_TORCH = 1,
    PAL_ALERT = 2,
    PAL_GOOD = 3,
    PAL_STONE = 4,
    PAL_MAIL = 5,
    PAL_IRON = 6,
    PAL_BANNER = 7,
    PAL_WOOD = 8,
    PAL_NIGHT = 9,
    PAL_SKY = 10,
    PAL_MIST = 11,
    PAL_ROAD = 12
};

struct Art {
    gs::Mipped leaf;
    gs::Mipped pier;
    gs::Mipped arch;
    gs::Mipped bar;
    gs::Mipped chain;
    gs::Mipped grate;
    gs::Mipped helm[2];
    gs::Mipped ram[2];
    gs::Mipped torch;
    gs::Mipped moon;
    gs::Mipped wedge;
    gs::Mipped spark;
    gs::Mipped banner;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace sally
