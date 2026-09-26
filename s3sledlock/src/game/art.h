// S3 SLED LOCK pictures. Drawn into VRAM at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace sledlock {

enum Pal : int {
    PAL_HUD = 0,
    PAL_AMBER = 1,
    PAL_RED = 2,
    PAL_GREEN = 3,
    PAL_SLED = 4,
    PAL_GATE = 5,
    PAL_SNOW = 6,
    PAL_STONE = 7,
    PAL_CABIN = 8,
    PAL_KEEP = 9,
    PAL_FX = 10,
    PAL_LAMP = 11,
    PAL_HARE = 12
};

constexpr int YAWS = 16;
constexpr int GATE_DIRS = 32;
// Nose-to-tail pixels of the sled, centered in each yaw bitmap.
constexpr int SLED_PX = 64;
// Timber length in the unrotated leaf bitmap.
constexpr int LEAF_PX = 62;

struct Art {
    gs::Mipped sled[YAWS];
    gs::Mipped leaf[GATE_DIRS];
    gs::Mipped snow, stone, drift, pine, cabin, keeper;
    gs::Mipped post, lamp, sign, hare, ladder;
    gs::Mipped flake, spray, crack, blob;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace sledlock
