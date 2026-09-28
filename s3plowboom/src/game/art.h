// S3 PLOW BOOM sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace plowboom {

enum Pal : int {
    PAL_HUD = 0,
    PAL_PLOW = 1,
    PAL_DRIVE = 2,
    PAL_BOOM = 3,
    PAL_BANK = 4,
    PAL_SPRAY = 5,
    PAL_SNOW = 6
};

struct Art {
    gs::Mipped plow[8];   // nose is north at heading 0
    gs::Mipped drive[8];  // the shaft the plow is delivering
    gs::Mipped post;
    gs::Mipped beam;
    gs::Mipped plank;
    gs::Mipped bank;
    gs::Mipped shed;
    gs::Mipped spray;
    int font[96] = {};
    int snowTile = 1;
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace plowboom
