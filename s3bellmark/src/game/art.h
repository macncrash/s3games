// Pictures drawn at boot. The gold lip is the only finished mark.
#pragma once

#include "console/gfx.h"
#include "console/vdp.h"

namespace bellmark {

enum Pal {
    PAL_HUD = 0,
    PAL_STONE = 1,
    PAL_BELL = 2,
    PAL_ROPE = 3,
    PAL_CLAP = 4,
    PAL_GOLD = 5,
    PAL_RINGER = 6,
    PAL_YOKE = 7
};

struct Art {
    gs::Mipped tower;
    gs::Mipped bell;
    gs::Mipped clapper;
    gs::Mipped rope;
    gs::Mipped notch;
    gs::Mipped ringer;
    gs::Image title;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace bellmark
