// S3 TOWERCLER pictures. Drawn into the S3-16 at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace tower {

enum Pal {
    PAL_HUD = 0,
    PAL_STONE = 1,
    PAL_KEEPER = 2,
    PAL_MOSS = 3,
    PAL_WOOD = 4,
    PAL_RUST = 5,
    PAL_GOLD = 6,
    PAL_DUST = 7,
    PAL_IVY = 8
};

struct Art {
    gs::Mipped tower;
    gs::Mipped hand[8];
    gs::Mipped keeper[2];
    gs::Mipped broom;
    gs::Mipped ivy, crate, stone, barrel, plank, lamp;
    gs::Mipped mote;
    gs::Mipped glyph[96];
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace tower
