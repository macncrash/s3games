// S3 HORN BELL pictures. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace hornbell {

constexpr int kTries = 3;
constexpr int kDieAt = 70;
constexpr int kSweetLo = 38;
constexpr int kSweetHi = 48;

enum Pal {
    PAL_HUD = 0,
    PAL_GOLD = 1,
    PAL_DIM = 2,
    PAL_BAD = 3,
    PAL_BRASS = 4,
    PAL_COAT = 5,
    PAL_YARD = 6,
    PAL_BELL = 7
};

struct Art {
    gs::Image player;
    gs::Image horn;
    gs::Image bell;
    gs::Image yoke;
    gs::Image lamp;
    gs::Image stand;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace hornbell
