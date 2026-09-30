// S3 BEACON COLUMN sprites. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace beacon {

enum Pal {
    PAL_TEXT = 0,
    PAL_GOLD = 1,
    PAL_ALERT = 2,
    PAL_GOOD = 3,
    PAL_CAR = 4,
    PAL_VAN = 5,
    PAL_TRUCK = 6,
    PAL_STONE = 7,
    PAL_BEAM = 8,
    PAL_TREE = 9,
    PAL_FX = 10,
    PAL_NIGHT = 11,
    PAL_ROAD = 12,
    PAL_LAMP = 13,
    PAL_TOWER = 14
};

struct Art {
    gs::Mipped car, van, truck;
    gs::Mipped tower, lamp, beam;
    gs::Mipped tree, rock, dust, shadow, star;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace beacon
