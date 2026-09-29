// S3 LOOM SEVEN pictures. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace loomseven {

constexpr int kGoal = 7;

enum Pal {
    PAL_HUD = 0,
    PAL_YOU = 1,
    PAL_DIM = 2,
    PAL_BAD = 3,
    PAL_HOUSE = 4,
    PAL_WOOD = 5,
    PAL_WARP = 6,
    PAL_SHUTTLE = 7
};

struct Art {
    gs::Image post;
    gs::Image beam;
    gs::Image warp;
    gs::Image weft;
    gs::Image shuttle;
    gs::Image reed;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace loomseven
