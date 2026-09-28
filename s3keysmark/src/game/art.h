// Pictures drawn at boot. Nothing is loaded from a file.
#pragma once

#include "console/gfx.h"
#include "console/vdp.h"

namespace keysmark {

enum Pal {
    PAL_INK = 0,
    PAL_GOLD = 1,
    PAL_IVORY = 2,
    PAL_MARK = 3,
    PAL_BAD = 4,
    PAL_WOOD = 5,
    PAL_NOTE = 6,
    PAL_PLAYED = 7,
    PAL_LAMP = 8,
    PAL_HAND = 9
};

struct Art {
    int font[96] = {};
    gs::Mipped note, key, staff, bar, lamp, hand;
    gs::Image logo, finished, open;
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace keysmark
