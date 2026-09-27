#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace spanpouc {

enum Pal {
    PAL_HUD = 0,
    PAL_WALKER = 1,
    PAL_POUCH = 2,
    PAL_TIMBER = 3,
    PAL_GATE = 4,
    PAL_SUN = 5,
    PAL_ROAD = 12
};

struct Art {
    gs::Image walker;
    gs::Image pouch;
    gs::Image post;
    gs::Image gate;
    gs::Image sun;
    gs::Image glyph[96];
    int glyphW[96] = {};
    int glyphH = 0;
    gs::Image title;
    gs::Image line1;
    gs::Image line2;
    gs::Image hint;
    gs::Image win;
    gs::Image fell;
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace spanpouc
