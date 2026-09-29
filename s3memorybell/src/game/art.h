// S3 MEMORYBELL pictures. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace memorybell {

enum Pal {
    PAL_B0 = 0,
    PAL_B1 = 1,
    PAL_B2 = 2,
    PAL_B3 = 3,
    PAL_INK = 4,
    PAL_GOLD = 5,
    PAL_ALERT = 6,
    PAL_OK = 7,
    PAL_DIM = 8,
    PAL_BEAM = 9
};

constexpr int NB = 4;
constexpr int SEQ = 4;
constexpr int TRIES = 3;

struct Art {
    gs::Mipped bell;
    gs::Mipped mark;
    gs::Mipped beam;
    gs::Mipped pip;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace memorybell
