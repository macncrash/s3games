// Mail van pass sprites. Drawn into VRAM at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace mailpass {

enum Pal : int {
    PAL_HUD = 0,
    PAL_ICE = 1,
    PAL_BAD = 2,
    PAL_GOOD = 3,
    PAL_VAN = 4,
    PAL_PINE = 5,
    PAL_CAIRN = 6,
    PAL_SIGN = 7,
    PAL_SACK = 8,
    PAL_ROAD = 12
};

struct Art {
    gs::Mipped hood, pine, cairn, sign, sack, arch;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace mailpass
