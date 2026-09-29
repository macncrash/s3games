// Night desk and wall clock, drawn into the VDP at boot.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace boardchime {

enum Pal {
    PAL_DESK = 0,
    PAL_LAMP = 1,
    PAL_PLUG = 2,
    PAL_CLOCK = 3,
    PAL_INK = 4,
    PAL_TITLE = 5,
    PAL_WIN = 6,
    PAL_DEAD = 7,
    PAL_HINT = 8,
    PAL_CORD = 9
};

constexpr int JACKS = 6;
constexpr int kNeed = 4;

struct Art {
    gs::Image desk;
    gs::Image lamp;
    gs::Image plug;
    gs::Image clock;
    gs::Image hand;
    gs::Image bead;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace boardchime
