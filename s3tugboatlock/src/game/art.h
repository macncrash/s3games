// S3 TUGBOAT LOCK sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace tuglock {

enum Pal : int {
    PAL_HUD = 0,
    PAL_AMBER = 1,
    PAL_RED = 2,
    PAL_GREEN = 3,
    PAL_TUG = 4,
    PAL_GATE = 5,
    PAL_BRICK = 6,
    PAL_BANK = 7,
    PAL_FX = 8,
    PAL_KEEP = 9,
    PAL_YARD = 10,
    PAL_RIVAL = 11,
    PAL_PORT = 12,
    PAL_STBD = 13,
    PAL_BIRD = 14
};

constexpr int YAWS = 16;
constexpr int GATE_DIRS = 40;
constexpr int TUG_PX = 86;   // nose-to-stern pixels in the tug bitmap
constexpr int LEAF_PX = 64;  // timber length in the unrotated leaf

struct Art {
    gs::Mipped tug[YAWS];
    gs::Mipped leaf[GATE_DIRS];
    gs::Mipped brick, path, field, reed;
    gs::Mipped post, buoy, lamp;
    gs::Mipped keeper, shed, crane, tank;
    gs::Mipped gull[2], smoke, wake, foam, blob;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace tuglock
