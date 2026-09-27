// S3 HARBOR BANN sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace hbann {

enum Pal : int {
    PAL_TEXT = 0,
    PAL_BOAT = 1,
    PAL_CUTTER = 2,
    PAL_BANNER = 3,
    PAL_FX = 4,
    PAL_AMBER = 5,
    PAL_RED = 6,
    PAL_DIM = 7,
    PAL_QUAY = 8,
    PAL_SKY = 9,
    PAL_PLATE = 10,
    PAL_WAKE = 11,
    PAL_ROAD = 12,
};

struct Art {
    gs::Mipped boat, cutter, raft, pier;
    gs::Image banner, wake, splash, buoy, lamp, sun, cloud, plate;
    gs::Image glyph[128];
    int cellW = 16;
    int cellH = 16;
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace hbann
