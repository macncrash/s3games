// Pictures for the aisle, drawn into the S3-16 at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace shelvechime {

enum Pal {
    PAL_INK = 0,
    PAL_GOLD = 1,
    PAL_ALERT = 2,
    PAL_OK = 3,
    PAL_DIM = 4,
    PAL_A = 5,
    PAL_B = 6,
    PAL_C = 7,
    PAL_D = 8,
    PAL_WOOD = 9,
    PAL_ROOM = 10,
    PAL_MARK = 11
};

inline int bookPal(int row) { return PAL_A + (row & 3); }

constexpr float ROW_Y0 = 52.f;
constexpr float ROW_DY = 42.f;
constexpr float CASE_CX = 228.f;
constexpr float CASE_CY = 118.f;
constexpr float PLANK_CX = 228.f;
constexpr float CART_X = 42.f;
constexpr float HAND_X = 98.f;
constexpr float PLATE_X = 132.f;
constexpr float RES_X = 186.f;
constexpr float SLOT_X0 = 244.f;
constexpr float SLOT_DX = 20.f;
constexpr float BOOK_DH = 30.f;
constexpr int BOOK_W = 16;
constexpr int BOOK_H = 30;
constexpr int PLATE_W = 22;
constexpr int PLATE_H = 26;

struct Art {
    gs::Mipped book[4];
    gs::Mipped plate[4];
    gs::Mipped cart;
    gs::Mipped plank;
    gs::Mipped rail;
    gs::Mipped residents;
    gs::Mipped caseBack;
    gs::Mipped bracket;
    gs::Mipped clock;
    gs::Mipped bell;
    gs::Mipped dot;
    gs::Mipped banner;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace shelvechime
