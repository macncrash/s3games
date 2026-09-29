// Palisade banner sprites. Drawn into the VDP at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace palbann {

enum Pal {
    PAL_HUD = 0,
    PAL_TIMBER = 1,
    PAL_CLOAK = 2,
    PAL_CLOTH = 3,
    PAL_RAID = 4,
    PAL_FIELD = 5,
    PAL_DITCH = 6,
    PAL_IRON = 7
};

struct Art {
    gs::Mipped stand, walkA, walkB, thrust;
    gs::Mipped raider[2];
    gs::Mipped cloth, pole, stake, gate, reed, shadow, mudpad;
    gs::Mipped glyph[96];
    int font[96] = {};
    int grass = 1;
    int mud = 2;
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace palbann
