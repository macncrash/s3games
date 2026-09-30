// S3 GRANARYPACE pictures. Drawn at boot. No asset files.
#pragma once

#include "console/gfx.h"
#include "console/vdp.h"

namespace granary {

constexpr int kPaces = 3;
constexpr int kStep = 28;
constexpr int kWindow = 90;
constexpr float kStride = 34.f;

enum Pal {
    PAL_BARN = 0,
    PAL_MAN = 1,
    PAL_GUN = 2,
    PAL_WHEAT = 3,
    PAL_DUST = 4,
    PAL_SKY = 5,
    PAL_INK = 10,
    PAL_GOLD = 11,
    PAL_BAD = 12,
    PAL_DIM = 13
};

struct Art {
    gs::Image barn;
    gs::Image manA;
    gs::Image manB;
    gs::Image gun;
    gs::Image flash;
    gs::Image stalk;
    gs::Image sack;
    gs::Image boot;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace granary
