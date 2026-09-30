// S3 PRESS BELL pictures. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace pressbell {

constexpr int kTries = 3;
constexpr int kDieAt = 80;
constexpr int kSweetLo = 48;
constexpr int kSweetHi = 58;

enum Pal {
    PAL_HUD = 0,
    PAL_GOLD = 1,
    PAL_DIM = 2,
    PAL_BAD = 3,
    PAL_IRON = 4,
    PAL_WOOD = 5,
    PAL_BELL = 6,
    PAL_PAPER = 7,
    PAL_INK = 8
};

struct Art {
    gs::Image frame;
    gs::Image platen;
    gs::Image sheet;
    gs::Image screw;
    gs::Image bell;
    gs::Image post;
    gs::Image roller;
    gs::Image lamp;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace pressbell
