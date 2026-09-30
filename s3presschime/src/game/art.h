// S3 PRESSCHIME pictures. Drawn at boot. No asset files.
// The platen's travel is long. Only the short span against the sheet can chime the hour.
#pragma once

#include "console/gfx.h"
#include "console/vdp.h"

namespace presschime {

constexpr int kStroke = 40;
constexpr int kShortLo = 18;
constexpr int kShortHi = 22;
constexpr int kFpc = 5;
constexpr int kGraceSec = 8;
constexpr int kHourSec = 12 * 3600;
constexpr int kStartSec = kHourSec - 20;

constexpr float kPi = 3.14159265f;
constexpr float kPressX = 196.f;
constexpr float kBedY = 168.f;

enum Pal {
    PAL_HUD = 0,
    PAL_GOLD = 1,
    PAL_DIM = 2,
    PAL_BAD = 3,
    PAL_IRON = 4,
    PAL_WOOD = 5,
    PAL_BELL = 6,
    PAL_PAPER = 7,
    PAL_INK = 8,
    PAL_FACE = 9
};

struct Art {
    gs::Image frame;
    gs::Image platen;
    gs::Image sheet;
    gs::Image screw;
    gs::Image clock;
    gs::Image pip;
    gs::Image roller;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace presschime
