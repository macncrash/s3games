// S3 BEACON CLER sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace bcler {

enum Pal {
    PAL_TEXT = 0,
    PAL_GOLD = 1,
    PAL_ALERT = 2,
    PAL_GOOD = 3,
    PAL_TOWER = 4,
    PAL_KEEPER = 5,
    PAL_KELP = 6,
    PAL_STEEL = 7,
    PAL_ROPE = 8,
    PAL_BUOY = 9,
    PAL_WOOD = 10,
    PAL_FX = 11,
    PAL_NIGHT = 12
};

struct Art {
    gs::Mipped keeper[2];
    gs::Mipped gaff;
    gs::Mipped kelp;
    gs::Mipped crate;
    gs::Mipped buoy;
    gs::Mipped rope;
    gs::Mipped drift;
    gs::Mipped lantern;
    gs::Mipped tower;
    gs::Mipped lamp;
    gs::Mipped beam;
    gs::Mipped rail;
    gs::Mipped spray;
    gs::Mipped shadow;
    gs::Mipped mark;
    gs::Mipped glyph[96];
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace bcler
