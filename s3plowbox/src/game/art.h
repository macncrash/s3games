// S3 PLOW BOX sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace plowbox {

enum Pal : int {
    PAL_HUD = 0,
    PAL_PLOW = 1,
    PAL_BOX = 2,
    PAL_BARN = 3,
    PAL_BANK = 4,
    PAL_SPRAY = 5,
    PAL_SNOW = 6
};

struct Art {
    gs::Mipped plow[16];  // 8 headings × blade up/down, nose is north at heading 0
    gs::Mipped stake;
    gs::Mipped tape;
    gs::Mipped barn;
    gs::Mipped bank;
    gs::Mipped spray;
    int font[96] = {};
    int snowTile = 1;
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace plowbox
