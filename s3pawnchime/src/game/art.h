// S3 PAWNCHIME — file, pawn, and clock drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace pawnchime {

constexpr int kSteps = 4;
constexpr int kLeadSec = 8;
constexpr int kHourSec = 12 * 3600;
constexpr int kStartSec = kHourSec - kLeadSec;
constexpr int kGraceSec = 4;

enum Pal {
    PAL_BOARD = 0,
    PAL_PAWN = 1,
    PAL_FOE = 2,
    PAL_BELL = 3,
    PAL_INK = 4,
    PAL_TITLE = 5,
    PAL_WIN = 6,
    PAL_BAD = 7,
    PAL_HINT = 8
};

struct Art {
    gs::Image board;
    gs::Image pawn;
    gs::Image foe;
    gs::Image bell;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace pawnchime
