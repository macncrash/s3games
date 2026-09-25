// S3 MEMORY pictures. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace memo {

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
constexpr int GAP_X = 8;
constexpr int GAP_Y = 4;
constexpr int GRID_W = COLS * CARD_W + (COLS - 1) * GAP_X;
constexpr int GRID_H = ROWS * CARD_H + (ROWS - 1) * GAP_Y;
constexpr int GRID_X = (gs::SCREEN_W - GRID_W) / 2;
constexpr int GRID_Y = 26;
constexpr int WOOD_T = 18;
constexpr int WOOD_B = 18;
constexpr int WOOD_SIDE = 52;
constexpr int SIDE_Y = WOOD_T;
constexpr int SIDE_H = gs::SCREEN_H - WOOD_T - WOOD_B;
constexpr int BOT_Y = gs::SCREEN_H - WOOD_B;

struct Art {
    gs::Mipped face[PAIRS];
    gs::Mipped back;
    gs::Mipped cursor;
    gs::Mipped pip;
    gs::Mipped bar;
    gs::Mipped woodTop;
    gs::Mipped woodBot;
    gs::Mipped woodSide;
    gs::Mipped logo;
    gs::Mipped lineA;
    gs::Mipped lineB;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace memo
