// Boot-time glyphs, the tape, and the drawer. No asset files.
#pragma once

#include "console/vdp.h"

namespace memorytape {

constexpr int LEN = 5;
constexpr int KINDS = 6;

constexpr int PAL_GLYPH = 1;
constexpr int PAL_CREAM = 2;
constexpr int PAL_GOLD = 3;
constexpr int PAL_DIM = 4;
constexpr int PAL_LEAF = 5;

struct Art {
    gs::Image glyph[KINDS];
    gs::Image tape;
    gs::Image drawer;
    gs::Image slot;
    gs::Image cursor;
    int font[96] = {};
    // The sequence the drawer has to recall.
    int tapeSeq[LEN] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace memorytape
