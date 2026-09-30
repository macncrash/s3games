// S3 CULVERT LADDER sprites. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace culvert {

enum Pal {
    PAL_TEXT = 0,
    PAL_AMBER = 1,
    PAL_ALERT = 2,
    PAL_GOOD = 3,
    PAL_MAN = 4,
    PAL_BRICK = 5,
    PAL_LADDER = 6,
    PAL_WATER = 7,
    PAL_LAMP = 8,
    PAL_IRON = 9,
    PAL_WALL = 11
};

struct Art {
    gs::Mipped man, stride, brick, rung, arch, drip, lamp;
    gs::Mipped glyph[96];
    int font[96] = {};
    int wallTile = 1;
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace culvert
