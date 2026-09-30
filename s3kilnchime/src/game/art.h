// S3 KILN CHIME pictures. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace kilnchime {

constexpr int kFires = 4;
constexpr int kHourSec = 12 * 3600;
constexpr int kLeadSec = 18;
constexpr int kStartSec = kHourSec - kLeadSec;
constexpr int kGraceSec = 4;

enum Pal {
    PAL_HUD = 0,
    PAL_BELL = 1,
    PAL_DIM = 2,
    PAL_BAD = 3,
    PAL_CLAY = 4,
    PAL_BRICK = 5,
    PAL_FIRE = 6,
    PAL_ASH = 7
};

struct Art {
    gs::Image kiln;
    gs::Image pot;
    gs::Image flame;
    gs::Image shelf;
    gs::Image bell;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace kilnchime
