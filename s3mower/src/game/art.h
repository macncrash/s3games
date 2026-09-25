// S3 MOWER pictures. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace mower {

enum Pal {
    PAL_HUD = 0,
    PAL_GRASS = 1,
    PAL_CUT = 2,
    PAL_YARD = 3,
    PAL_MOWER = 4,
    PAL_TREE = 5,
    PAL_SHED = 6,
    PAL_SKY = 7,
    PAL_FLOWER = 8,
    PAL_GNOME = 9,
    PAL_TITLE = 10,
    PAL_SUN = 11,
    PAL_ALERT = 12
};

struct Art {
    gs::Mipped mower[16];
    gs::Mipped shadow;
    gs::Mipped shed, tree, mail, gnome;
    gs::Mipped flower[3];
    gs::Mipped cloud, rain, sun;
    gs::Mipped logo, tag, hint, turn;
    gs::Mipped win, lose, stripe;
    gs::Mipped plaque;
    int tall[3] = {};
    int cutH = 1, cutD = 1, cutV = 1, cutVD = 1;
    int rail = 1, post = 1, gravel = 1, bar = 1;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace mower
