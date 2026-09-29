// S3 RICKSHAW PLAT pictures. Drawn into sprite ROM at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace rickplat {

enum Pal {
    PAL_HUD = 0,
    PAL_AMBER = 1,
    PAL_BAD = 2,
    PAL_GOOD = 3,
    PAL_CAB = 4,
    PAL_STONE = 5,
    PAL_ROAD = 6,
    PAL_WALL = 7,
    PAL_TREE = 8,
    PAL_SKY = 9,
    PAL_MARK = 10,
    PAL_MAN = 11,
    PAL_DUST = 12,
    PAL_BIRD = 13,
    PAL_LAMP = 14,
    PAL_AWN = 15
};

struct Art {
    gs::Mipped cab, wheel, pull[2], rider;
    gs::Mipped stone, lip, stripe, wall, tree, lamp, awn;
    gs::Mipped cloud, sun, bird[2], dust;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace rickplat
