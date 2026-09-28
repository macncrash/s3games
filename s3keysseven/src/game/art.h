// Pictures for the keys, drawn into the VDP at boot. No asset files.
#pragma once

#include "console/gfx.h"
#include "console/vdp.h"

namespace keysseven {

constexpr int kLanes = 4;
constexpr int kLaneX[kLanes] = {64, 128, 192, 256};
constexpr int kHitY = 164;
constexpr int kRace = 7;
constexpr int kBar = 4;
constexpr int kNotes = kRace * kBar;

enum Pal {
    PAL_INK = 0,
    PAL_IVORY = 1,
    PAL_EBONY = 2,
    PAL_NOTE = 3,
    PAL_GOLD = 4,
    PAL_RIVAL = 5,
    PAL_WOOD = 6,
    PAL_LAMP = 7,
    PAL_MISS = 8
};

struct Art {
    int font[96] = {};
    gs::Mipped note, key, black, lamp, glow;
    gs::Image title, leave;
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace keysseven
