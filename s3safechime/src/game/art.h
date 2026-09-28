// Safe, dial, clock, and bell, drawn into the VDP at boot. No asset files.
#pragma once

#include "console/gfx.h"
#include "console/vdp.h"

namespace safechime {

constexpr int kNotches = 40;
constexpr int kStops = 3;
constexpr int kFpc = 6;
constexpr int kGraceSec = 24;
constexpr int kHourSec = 12 * 3600;
constexpr int kStartSec = kHourSec - 150;

enum Pal {
    PAL_INK = 0,
    PAL_STEEL = 1,
    PAL_DOOR = 2,
    PAL_DIAL = 3,
    PAL_GOLD = 4,
    PAL_BELL = 5,
    PAL_FACE = 6,
    PAL_DEAD = 7,
    PAL_LAMP = 8
};

struct Art {
    int font[96] = {};
    gs::Mipped body, door, dial, needle, tick, lamp, bell, face, pip, handle;
    gs::Image title, rule;
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace safechime
