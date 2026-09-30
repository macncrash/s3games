// S3 INKWELL CHIME pictures. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace inkwellchime {

constexpr int kDips = 4;
constexpr int kTries = 3;
constexpr int kHourSec = 12 * 3600;
constexpr int kLeadSec = 36;
constexpr int kGraceSec = 6;
constexpr int kFpc = 3;
constexpr int kPeriod = 22;

enum Pal {
    PAL_HUD = 0,
    PAL_BRASS = 1,
    PAL_DIM = 2,
    PAL_BAD = 3,
    PAL_INK = 4,
    PAL_DESK = 5,
    PAL_QUILL = 6,
    PAL_PAGE = 7,
    PAL_CLOCK = 8,
    PAL_GOLD = 9
};

struct Art {
    gs::Image desk;
    gs::Image page;
    gs::Image well;
    gs::Image quill;
    gs::Image drop;
    gs::Image face;
    gs::Image dot;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace inkwellchime
