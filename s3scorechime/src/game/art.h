// S3 SCORE CHIME pictures. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace scorechime {

constexpr int kMarks = 4;
constexpr int kHourSec = 12 * 3600;
constexpr int kLeadSec = 18;
constexpr int kStartSec = kHourSec - kLeadSec;
constexpr int kGraceSec = 4;

enum Pal {
    PAL_HUD = 0,
    PAL_BELL = 1,
    PAL_DIM = 2,
    PAL_BAD = 3,
    PAL_CHALK = 4,
    PAL_BOARD = 5,
    PAL_BALL = 6,
    PAL_PITCH = 7
};

struct Art {
    gs::Image board;
    gs::Image tick;
    gs::Image ball;
    gs::Image line;
    gs::Image bell;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace scorechime
