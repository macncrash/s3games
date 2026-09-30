// S3 CULVERT POUC pictures. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace culvertpouc {

enum Pal {
    PAL_HUD = 0,
    PAL_STONE = 1,
    PAL_FLOW = 2,
    PAL_POUCH = 3,
    PAL_PLAYER = 4,
    PAL_GATE = 5,
    PAL_ROAD = 6,
    PAL_ALERT = 7,
    PAL_LAMP = 8
};

struct Art {
    gs::Mipped stand, runA, runB, duck, leap;
    gs::Mipped pouch[2];
    gs::Mipped ring, soffit, gate, lip, water, weed;
    gs::Mipped deck, truck, lamp, flame[2];
    gs::Mipped mark, shadow;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace culvertpouc
