// S3 INKWELL BELL pictures. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace inkwellbell {

constexpr int kMarks = 8;
constexpr int kTries = 3;

enum Pal {
    PAL_HUD = 0,
    PAL_BRASS = 1,
    PAL_DIM = 2,
    PAL_BAD = 3,
    PAL_INK = 4,
    PAL_DESK = 5,
    PAL_QUILL = 6,
    PAL_PAGE = 7,
    PAL_BELL = 8
};

struct Art {
    gs::Image desk;
    gs::Image page;
    gs::Image well;
    gs::Image quill;
    gs::Image drop;
    gs::Image bell;
    gs::Image nib;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace inkwellbell
