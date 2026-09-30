// S3 BEACON WELL sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace bwell {

enum Pal {
    PAL_HUD = 0,
    PAL_LAMP = 1,
    PAL_SEA = 2,
    PAL_STONE = 3,
    PAL_KEEPER = 4,
    PAL_TIDE = 5,
    PAL_WICK = 6,
    PAL_SPARK = 7,
    PAL_NIGHT = 8
};

struct Art {
    gs::Mipped tower;
    gs::Mipped well;
    gs::Mipped keeper[2];
    gs::Mipped tide[2];
    gs::Mipped wick[2];
    gs::Mipped beam;
    gs::Mipped spark;
    gs::Mipped rock;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace bwell
