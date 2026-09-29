// Felt, cards, and the bell. Drawn into VRAM at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace solitairebell {

enum Pal {
    PAL_FELT = 0,
    PAL_CARD = 1,
    PAL_PIP = 2,
    PAL_BELL = 3,
    PAL_INK = 4,
    PAL_TITLE = 5,
    PAL_WIN = 6,
    PAL_BAD = 7,
    PAL_HINT = 8
};

struct Art {
    gs::Image felt;
    gs::Image card;
    gs::Image pip;
    gs::Image bell;
    gs::Image clapper;
    gs::Image pile;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace solitairebell
