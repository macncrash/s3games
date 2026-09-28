// Pictures for the keys and the bell, drawn into the VDP at boot. No asset files.
#pragma once

#include "console/gfx.h"
#include "console/vdp.h"

namespace keysbell {

constexpr int kLanes = 4;
constexpr int kLaneX[kLanes] = {72, 120, 200, 248};
constexpr int kHitY = 168;
constexpr int kPhrase = 8;
constexpr int kTries = 3;

enum Pal {
    PAL_INK = 0,
    PAL_IVORY = 1,
    PAL_EBONY = 2,
    PAL_NOTE = 3,
    PAL_GOLD = 4,
    PAL_BELL = 5,
    PAL_WOOD = 6,
    PAL_DEAD = 7,
    PAL_CLAP = 8
};

struct Art {
    int font[96] = {};
    gs::Mipped note, key, black, bell, glow, mark;
    gs::Image title, rule;
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace keysbell
