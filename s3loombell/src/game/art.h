// S3 LOOMBELL pictures. Drawn at boot. No asset files.
#pragma once

#include "console/gfx.h"
#include "console/vdp.h"

namespace loombell {

constexpr int kTries = 3;
constexpr int kDieAt = 72;
constexpr int kSweetLo = 40;
constexpr int kSweetHi = 50;
constexpr int kPicks = 5;
constexpr int kWarps = 7;

enum Pal {
    PAL_WOOD = 0,
    PAL_WARP = 1,
    PAL_SHUTTLE = 2,
    PAL_BELL = 3,
    PAL_INK = 4,
    PAL_GOLD = 5,
    PAL_BAD = 6,
    PAL_DIM = 7,
    PAL_LAMP = 8,
    PAL_CLOTH = 9
};

struct Art {
    gs::Image frame;
    gs::Image warp;
    gs::Image shuttle;
    gs::Image reed;
    gs::Image heddle;
    gs::Image pick;
    gs::Image bell;
    gs::Image clapper;
    gs::Image lamp;
    gs::Image beam;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace loombell
