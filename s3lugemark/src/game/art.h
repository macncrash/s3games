// S3 LUGE MARK sprites. Drawn into VRAM and sprite ROM at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace luge {

enum Pal {
    PAL_HUD = 0,
    PAL_ICE = 1,
    PAL_MARK = 2,
    PAL_RIDER = 3,
    PAL_CREW = 4,
    PAL_TREE = 5,
    PAL_FIELD = 12
};

struct Art {
    gs::Mipped sled;
    gs::Mipped rider;
    gs::Mipped spray;
    gs::Mipped flag;
    gs::Mipped tree;
    gs::Mipped paint;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace luge
