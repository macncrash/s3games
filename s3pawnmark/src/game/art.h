// S3 PAWNMARK pictures. Drawn into the VDP at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace pawnmark {

enum Pal {
    PAL_INK = 0,
    PAL_WOOD = 1,
    PAL_GOLD = 2,
    PAL_IVORY = 3,
    PAL_NIGHT = 4,
    PAL_OK = 5,
    PAL_BAD = 6,
    PAL_FELT = 7
};

constexpr int FILES = 4;
constexpr int RANKS = 5;
constexpr int MARK_FILE = 2;
constexpr int SQ = 32;
constexpr int BOARD_X = 96;
constexpr int BOARD_Y = 48;

inline int sqX(int f) { return BOARD_X + f * SQ + SQ / 2; }
inline int sqY(int r) { return BOARD_Y + r * SQ + SQ / 2; }

struct Art {
    gs::Mipped pawn;
    gs::Mipped foe;
    gs::Mipped square;
    gs::Mipped mark;
    gs::Mipped crown;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace pawnmark
