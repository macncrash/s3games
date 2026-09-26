// S3 TUGBOAT PLAT sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace tugplat {

enum Pal : int {
    PAL_HUD = 0,
    PAL_AMBER = 1,
    PAL_RED = 2,
    PAL_GREEN = 3,
    PAL_TUG = 4,
    PAL_RIVAL = 5,
    PAL_TIMBER = 6,
    PAL_SHED = 7,
    PAL_FX = 8,
    PAL_BIRD = 9,
    PAL_CRANE = 10,
    PAL_LAMP = 11,
    PAL_YARD = 12,
    PAL_BUOY = 13,
    PAL_MARK = 14
};

constexpr int kYaws = 32;
constexpr float kPaintLen = 64.f;

struct Art {
    gs::Mipped tug[kYaws];
    gs::Mipped platform, plank, yard;
    gs::Mipped pile, crane, shed, buoy;
    gs::Mipped gull[2];
    gs::Mipped smoke, wake, shadow;
    gs::Mipped diamond, lamp, line;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace tugplat
