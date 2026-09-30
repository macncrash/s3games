// S3 HORNCHIME pictures. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace hornchime {

// Clock runs a little fast so a person can wait out one minute of dusk.
constexpr int kFpc = 10;
constexpr int kStartSec = 11 * 3600 + 59 * 60 + 54;  // 11:59:54
constexpr int kHourSec = 12 * 3600;                  // 12:00:00
constexpr int kGraceSec = 4;
constexpr int kNote = 24;
constexpr int kBreaths = 2;

enum Pal {
    PAL_HUD = 0,
    PAL_GOLD = 1,
    PAL_DIM = 2,
    PAL_BAD = 3,
    PAL_BRASS = 4,
    PAL_COAT = 5,
    PAL_STONE = 6,
    PAL_FACE = 7,
    PAL_SKY = 8
};

struct Art {
    gs::Image player;
    gs::Image horn;
    gs::Image tower;
    gs::Image face;
    gs::Image bell;
    gs::Image dot;
    gs::Image bar;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace hornchime
