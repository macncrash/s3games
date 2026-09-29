// S3 MEMORY SEVEN pictures. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace memoryseven {

enum Pal {
    PAL_TEXT = 0,
    PAL_AMBER = 1,
    PAL_RED = 2,
    PAL_GREEN = 3,
    PAL_TABLE = 4,
    PAL_BACK = 5,
    PAL_FACE = 6,
    PAL_SYM = 7,
    PAL_GOLD = 8,
    PAL_SHADE = 9
};

constexpr int COLS = 4;
constexpr int ROWS = 4;
constexpr int N = COLS * ROWS;
constexpr int KINDS = 8;

struct Art {
    gs::Mipped sym[KINDS];
    gs::Mipped back;
    gs::Mipped solid;
    int font[96] = {};
    float x0 = 56;
    float y0 = 36;
    float cw = 46;
    float ch = 36;
    float gx = 8;
    float gy = 6;
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace memoryseven
