#pragma once

#include "console/gfx.h"
#include "console/vdp.h"

namespace solitaire {

constexpr int kCards = 13;
constexpr int kCardW = 40;
constexpr int kCardH = 54;

enum Pal {
    PAL_FELT = 0,
    PAL_CARD = 1,
    PAL_INK = 2,
    PAL_GOLD = 3,
    PAL_TITLE = 4,
    PAL_WIN = 5,
    PAL_DIM = 6
};

struct Art {
    gs::Image card[kCards];
    gs::Image cursor;
    gs::Image glyph[96];
    gs::Image heart;
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace solitaire
