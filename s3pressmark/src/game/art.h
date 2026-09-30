// Pictures drawn at boot. The gold line is the only finished mark.
#pragma once

#include "console/gfx.h"
#include "console/vdp.h"

namespace pressmark {

enum Pal {
    PAL_HUD = 0,
    PAL_WOOD = 1,
    PAL_PAPER = 2,
    PAL_BRASS = 3,
    PAL_IRON = 4,
    PAL_INK = 5,
    PAL_MAN = 6,
    PAL_WIN = 7
};

struct Art {
    gs::Mipped frame;
    gs::Mipped platen;
    gs::Mipped bed;
    gs::Mipped sheet;
    gs::Mipped type;
    gs::Mipped roller;
    gs::Mipped figure;
    gs::Mipped mark;
    gs::Mipped gauge;
    gs::Mipped needle;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace pressmark
