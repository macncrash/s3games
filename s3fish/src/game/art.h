// S3 FISH sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace fish {

enum Pal {
    PAL_INK = 0,
    PAL_AMBER = 1,
    PAL_ALERT = 2,
    PAL_GOOD = 3,
    PAL_DIM = 4,
    PAL_BOAT = 5,
    PAL_BASS = 6,
    PAL_TROUT = 7,
    PAL_PIKE = 8,
    PAL_WALLEYE = 9,
    PAL_CAT = 10,
    PAL_FX = 11,
    PAL_WATER = 12,
    PAL_TREE = 13,
    PAL_SUN = 14,
    PAL_SUNSET = 15
};

struct Art {
    gs::Mipped fish[5][2];  // species, 0 short, 1 keeper (gold mark)
    gs::Mipped angler, boat, lure, line, spark, sun, moon, cloud;
    gs::Mipped tree, pine, weed, rock, lily, bubble, splash, gleam, star, shadow;
    gs::Mipped bird[2];
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace fish
