// S3 BELL SEVEN pictures. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace bellseven {

constexpr int kSeven = 7;
constexpr int kYourFace = 2;
constexpr int kTheirFace = 1;

enum Pal {
    PAL_HUD = 0,
    PAL_GOLD = 1,
    PAL_DIM = 2,
    PAL_BAD = 3,
    PAL_CREAM = 4,
    PAL_BRONZE = 5,
    PAL_ROPE = 6,
    PAL_YOU = 7,
    PAL_THEM = 8,
    PAL_STONE = 9,
    PAL_PEG = 10
};

struct Art {
    gs::Image belfry;
    gs::Image bell;
    gs::Image clapper;
    gs::Image rope;
    gs::Image you;
    gs::Image them;
    gs::Image wave;
    gs::Image peg;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace bellseven
