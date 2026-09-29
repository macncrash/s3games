// S3 MEMORY GOLD pictures. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace memorygold {

enum Pal {
    PAL_INK = 0,
    PAL_FACE = 1,
    PAL_BACK = 2,
    PAL_WOOD = 3,
    PAL_MARK = 4,
    PAL_GOLD = 5,
    PAL_ALERT = 6,
    PAL_OK = 7,
    PAL_DIM = 8
};

constexpr int CARD_W = 42;
constexpr int CARD_H = 40;
constexpr int COLS = 4;
constexpr int ROWS = 4;
constexpr int CARDS = COLS * ROWS;
constexpr int PAIRS = CARDS / 2;
constexpr int GOLD_FACES = 3;  // faces 0..2 count double
constexpr int LINE = 6;        // leave only when a gold double meets this
constexpr int GAP_X = 8;
constexpr int GAP_Y = 4;
constexpr int GRID_W = COLS * CARD_W + (COLS - 1) * GAP_X;
constexpr int GRID_H = ROWS * CARD_H + (ROWS - 1) * GAP_Y;
constexpr int GRID_X = (gs::SCREEN_W - GRID_W) / 2;
constexpr int GRID_Y = 28;

inline bool goldFace(int face) { return face >= 0 && face < GOLD_FACES; }

struct Art {
    gs::Mipped face[PAIRS];
    gs::Mipped back;
    gs::Mipped cursor;
    gs::Mipped pip;
    gs::Mipped bar;
    gs::Mipped wood;
    gs::Mipped logo;
    gs::Mipped lineA;
    gs::Mipped lineB;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace memorygold
