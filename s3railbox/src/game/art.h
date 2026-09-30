// S3 RAILBOX pictures. Drawn into VRAM at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace railbox {

constexpr int BOX_W = 168;

enum Pal : int {
    PAL_TEXT = 0,
    PAL_DIM = 1,
    PAL_RED = 2,
    PAL_GREEN = 3,
    PAL_AMBER = 4,
    PAL_CAB = 5,
    PAL_BOX = 6,
    PAL_LAND = 7,
    PAL_CREW = 8,
    PAL_FX = 9
};

struct Art {
    gs::Image cab[2];
    gs::Image box, rail, tree, hill, cloud, clock, puff, sun;
    gs::Image logo, tag, banIn, banCrew, banShort, banRan;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace railbox
