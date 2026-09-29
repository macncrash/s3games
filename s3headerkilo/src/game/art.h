// S3 HEADER KILO pictures. Drawn into sprite ROM at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace headerkilo {

enum Pal {
    PAL_INK = 0,
    PAL_GOLD = 1,
    PAL_HULL = 2,
    PAL_SAIL = 3,
    PAL_WHEEL = 4,
    PAL_MILL = 5,
    PAL_REED = 6,
    PAL_MARK = 7,
    PAL_WATER = 12
};

struct Art {
    gs::Image glyph[96];
    gs::Mipped hull;
    gs::Mipped sail;
    gs::Mipped wheel[3];
    gs::Mipped paddle;
    gs::Mipped mill;
    gs::Mipped reed;
    gs::Mipped buoy;
    gs::Mipped flag;
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace headerkilo
