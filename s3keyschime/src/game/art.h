// Keys, clock face, and bell, drawn into the VDP at boot. No asset files.
#pragma once

#include "console/gfx.h"
#include "console/vdp.h"

namespace keyschime {

constexpr int kLanes = 4;
constexpr int kLaneX[kLanes] = {64, 112, 208, 256};
constexpr int kHitY = 172;
constexpr int kPhrase = 5;
// Clock opens at 11:59:56. Four real seconds later the hour strikes.
constexpr int kOpenSec = 11 * 3600 + 59 * 60 + 56;
constexpr int kHourFrame = 4 * 60;

enum Pal {
    PAL_INK = 0,
    PAL_IVORY = 1,
    PAL_EBONY = 2,
    PAL_NOTE = 3,
    PAL_GOLD = 4,
    PAL_BELL = 5,
    PAL_NIGHT = 6,
    PAL_DEAD = 7,
    PAL_FACE = 8
};

struct Art {
    int font[96] = {};
    gs::Mipped note, key, black, bell, face, pip, glow;
    gs::Image title, rule;
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace keyschime
