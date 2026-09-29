// Felt, cards, clock, and the hour bell. Drawn into VRAM at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace solitairechime {

enum Pal {
    PAL_FELT = 0,
    PAL_CARD = 1,
    PAL_PIP = 2,
    PAL_BELL = 3,
    PAL_CLOCK = 4,
    PAL_INK = 5,
    PAL_TITLE = 6,
    PAL_WIN = 7,
    PAL_BAD = 8,
    PAL_HINT = 9
};

constexpr int kCards = 8;
constexpr int kOpenSec = 11 * 3600 + 59 * 60 + 52;
constexpr int kHourFrame = 8 * 60;

struct Art {
    gs::Image felt;
    gs::Image card;
    gs::Image pip;
    gs::Image bell;
    gs::Image clapper;
    gs::Image clock;
    gs::Image hand;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace solitairechime
