// S3 MILL POUC pictures. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace mpouc {

enum Pal {
    PAL_HUD = 0,
    PAL_STONE = 1,
    PAL_SAIL = 2,
    PAL_WATER = 3,
    PAL_MILLER = 4,
    PAL_POUCH = 5,
    PAL_WOOD = 6,
    PAL_WHEAT = 7,
    PAL_ALERT = 8,
    PAL_OK = 9
};

struct Art {
    gs::Mipped mill, cap, sail, bucket, hub;
    gs::Mipped miller, millerJ, pouch, sack, plank, door, reed;
    gs::Image glyph[96];
    int gw[96] = {};
    int gh = 8;
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace mpouc
