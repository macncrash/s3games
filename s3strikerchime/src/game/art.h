// Carnival striker, clock, and bell, drawn into the VDP at boot. No asset files.
#pragma once

#include "console/gfx.h"
#include "console/vdp.h"

namespace strikerchime {

// Clock opens at 11:59:56. The puck that leaves on the notch lands as the hour strikes.
constexpr int kOpenSec = 11 * 3600 + 59 * 60 + 56;
constexpr int kHourFrame = 4 * 60;
constexpr int kFlight = 48;
constexpr int kRelease = kHourFrame - kFlight;

enum Pal {
    PAL_HUD = 0,
    PAL_WOOD = 1,
    PAL_BRASS = 2,
    PAL_MAN = 3,
    PAL_NIGHT = 4,
    PAL_PUCK = 5,
    PAL_GOLD = 6,
    PAL_FACE = 7
};

struct Art {
    int font[96] = {};
    gs::Mipped tower, bell, puck, man, mallet, face, hand, pip, moon, ground;
    gs::Image title, rule;
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace strikerchime
