// S3 PRESS SEVEN pictures. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace pressseven {

constexpr int kSeven = 7;
constexpr int kGoldFace = 2;
constexpr int kCreamFace = 1;

enum Pal {
    PAL_HUD = 0,
    PAL_GOLD = 1,
    PAL_DIM = 2,
    PAL_BAD = 3,
    PAL_CREAM = 4,
    PAL_WOOD = 5,
    PAL_IRON = 6,
    PAL_INK = 7,
    PAL_YOU = 8,
    PAL_RIVAL = 9,
    PAL_PAPER = 10
};

struct Art {
    gs::Image frame;
    gs::Image platen;
    gs::Image bed;
    gs::Image sheetGold;
    gs::Image sheetCream;
    gs::Image screw;
    gs::Image printer;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace pressseven
