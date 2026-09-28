// S3 TOWER LADD pictures. Drawn into the S3-16 at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace towerladd {

enum Pal {
    PAL_HUD = 0,
    PAL_STONE = 1,
    PAL_KEEP = 2,
    PAL_IRON = 3,
    PAL_TORCH = 4,
    PAL_CLOTH = 5,
    PAL_OK = 6,
    PAL_ALERT = 7,
    PAL_DUSK = 8,
    PAL_PIT = 9
};

struct Art {
    gs::Mipped stand, walkA, walkB, jump, climbA, climbB;
    gs::Mipped ladder, merlon, torch, banner, moon, hatch;
    gs::Image glyph[96];
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace towerladd
