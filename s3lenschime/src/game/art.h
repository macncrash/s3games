// S3 LENSCHIME pictures. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace lenschime {

constexpr int kPlates = 3;
constexpr int kGraceSec = 16;
constexpr int kFpc = 4;
constexpr int kLeadSec = 22;
constexpr int kHourSec = 12 * 3600;

enum Pal {
    PAL_HUD = 0,
    PAL_STONE = 1,
    PAL_CLOCK = 2,
    PAL_BRASS = 3,
    PAL_GLASS = 4,
    PAL_BELL = 5,
    PAL_OK = 6,
    PAL_BAD = 7
};

struct Art {
    gs::Mipped tower, face, bell, clapper, beam;
    gs::Mipped body, barrel, glass, corner, caret, plate, dot, hand;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace lenschime
