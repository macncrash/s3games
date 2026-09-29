// Pictures drawn into sprite ROM at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace mushslip {

enum Pal : int {
    PAL_HUD = 0,
    PAL_TEAM = 1,
    PAL_RIVAL = 2,
    PAL_WOOD = 3,
    PAL_LAMP = 4,
    PAL_FLAG = 5,
    PAL_WIN = 6,
    PAL_ALERT = 7,
    PAL_INK = 8
};

struct Art {
    gs::Mipped sled[8];
    gs::Mipped post;
    gs::Mipped lamp;
    gs::Mipped flag;
    gs::Mipped puff;
    gs::Mipped title;
    gs::Mipped sub;
    gs::Mipped berthed;
    gs::Mipped beaten;
    gs::Mipped tide;
    gs::Mipped paused;
    gs::Image glyph[96];
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace mushslip
