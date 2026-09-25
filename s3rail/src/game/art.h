// S3 RAIL pictures. Drawn into VRAM at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace rail {

// The painted stop is this wide, and the nose has to halt inside it.
constexpr int BOX_W = 120;

enum Pal : int {
    PAL_TEXT = 0,
    PAL_DIM = 1,
    PAL_RED = 2,
    PAL_GREEN = 3,
    PAL_AMBER = 4,
    PAL_CAB = 5,
    PAL_STATION = 6,
    PAL_SCENERY = 7,
    PAL_GO = 8,
    PAL_STOP = 9,
    PAL_FX = 10,
    PAL_PANEL = 11
};

struct Art {
    gs::Image loco[2];
    gs::Image stop[4];
    gs::Image rail, tree, hill, cloud, signal, board, puff, sun, pole, panel;
    gs::Image logo, tag, banOn, banLate, banMade, banRan;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace rail
