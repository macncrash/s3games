// S3 LOOMCHIME pictures. Drawn at boot. No asset files.
#pragma once

#include "console/gfx.h"
#include "console/vdp.h"

namespace loomchime {

constexpr int kShuttles = 3;
constexpr int kDieAt = 80;
constexpr int kSweetLo = 42;
constexpr int kSweetHi = 54;
constexpr int kGraceSec = 18;
constexpr int kFpc = 6;
constexpr int kLeadSec = 20;
constexpr int kHourSec = 12 * 3600;
constexpr int kWarps = 7;
constexpr int kPicks = 6;

enum Pal {
    PAL_WOOD = 0,
    PAL_WARP = 1,
    PAL_SHUTTLE = 2,
    PAL_BELL = 3,
    PAL_CLOTH = 4,
    PAL_CLOCK = 5,
    PAL_HAND = 6,
    PAL_LAMP = 7,
    PAL_INK = 10,
    PAL_GOLD = 11,
    PAL_BAD = 12,
    PAL_DIM = 13
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
    gs::Image face;
    gs::Image dot;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace loomchime
