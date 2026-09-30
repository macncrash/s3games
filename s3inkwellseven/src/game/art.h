// S3 INKWELL SEVEN pictures. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace inkwellseven {

constexpr int kSeven = 7;
constexpr int kGoldFace = 2;
constexpr int kCreamFace = 1;

enum Pal {
    PAL_HUD = 0,
    PAL_GOLD = 1,
    PAL_DIM = 2,
    PAL_BAD = 3,
    PAL_CREAM = 4,
    PAL_DESK = 5,
    PAL_QUILL = 6,
    PAL_YOU = 7,
    PAL_RIVAL = 8,
    PAL_PAGE = 9
};

struct Art {
    gs::Image desk;
    gs::Image page;
    gs::Image well;
    gs::Image pool;
    gs::Image quill;
    gs::Image bead;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace inkwellseven
