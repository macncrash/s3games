// Pictures drawn at boot. Wet ink is the only finished mark.
#pragma once

#include "console/gfx.h"
#include "console/vdp.h"

namespace inkwellmark {

enum Pal {
    PAL_HUD = 0,
    PAL_DESK = 1,
    PAL_INK = 2,
    PAL_QUILL = 3,
    PAL_PAGE = 4,
    PAL_HAND = 5,
    PAL_BRASS = 6,
    PAL_BLOT = 7
};

struct Art {
    gs::Mipped desk;
    gs::Mipped well;
    gs::Mipped quill;
    gs::Mipped page;
    gs::Mipped stroke;
    gs::Mipped blot;
    gs::Mipped hand;
    gs::Image title;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace inkwellmark
