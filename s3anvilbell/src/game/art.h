// S3 ANVIL BELL pictures. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace anvilbell {

constexpr int kTries = 3;
constexpr int kDieAt = 72;
constexpr int kSweetLo = 40;
constexpr int kSweetHi = 50;

enum Pal {
    PAL_HUD = 0,
    PAL_GOLD = 1,
    PAL_DIM = 2,
    PAL_BAD = 3,
    PAL_IRON = 4,
    PAL_WOOD = 5,
    PAL_BELL = 6,
    PAL_NIGHT = 7
};

struct Art {
    gs::Image anvil;
    gs::Image hammer;
    gs::Image tower;
    gs::Image bell;
    gs::Image slug;
    gs::Image lamp;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace anvilbell
