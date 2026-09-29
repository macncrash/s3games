// S3 ANVIL SEVEN pictures. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace anvilseven {

constexpr int kSeven = 7;
constexpr int kGoldFace = 2;
constexpr int kCreamFace = 1;

enum Pal {
    PAL_HUD = 0,
    PAL_GOLD = 1,
    PAL_DIM = 2,
    PAL_BAD = 3,
    PAL_CREAM = 4,
    PAL_IRON = 5,
    PAL_FIRE = 6,
    PAL_SMITH = 7,
    PAL_RIVAL = 8
};

struct Art {
    gs::Image anvil;
    gs::Image hammer;
    gs::Image barGold;
    gs::Image barCream;
    gs::Image spark;
    gs::Image smith;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace anvilseven
