// S3 GRASS sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace grass {

enum Pal {
    PAL_HUD = 0,
    PAL_AMBER = 1,
    PAL_BAD = 2,
    PAL_GOOD = 3,
    PAL_SHIP = 4,
    PAL_TREE = 5,
    PAL_MARK = 6,
    PAL_BARN = 7,
    PAL_SKY = 8,
    PAL_DUST = 9,
    PAL_BIRD = 10,
    PAL_CONE = 11,
    PAL_FIELD = 12
};

struct Art {
    gs::Mipped cub[3];
    gs::Mipped tree[2];
    gs::Mipped post;
    gs::Mipped sock[3];
    gs::Mipped hangar;
    gs::Mipped cone;
    gs::Mipped bar;
    gs::Mipped cloud;
    gs::Mipped sun;
    gs::Mipped hill;
    gs::Mipped bird[2];
    gs::Mipped dust;
    gs::Mipped shadow;
    gs::Mipped chevron;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace grass
