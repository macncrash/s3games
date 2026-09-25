// S3 SHELVE pictures. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace shelve {

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

// The aisle the pictures were drawn to fit. 320x224, HUD on rows 0 and 26-27.
constexpr float ROW_Y0 = 46.f;
constexpr float ROW_DY = 44.f;
constexpr float CASE_CX = 230.f;
constexpr float CASE_CY = 114.f;
constexpr float PLANK_CX = 230.f;
constexpr float CART_X = 40.f;
constexpr float HAND_X = 96.f;
constexpr float PLATE_X = 128.f;
constexpr float RES_X = 190.f;
constexpr float SLOT_X0 = 246.f;
constexpr float SLOT_DX = 22.f;
constexpr float BOOK_DH = 32.f;
constexpr int CASE_W = 168;
constexpr int CASE_H = 180;
constexpr int BOOK_W = 18;
constexpr int BOOK_H = 32;
constexpr int PLANK_W = 152;
constexpr int PLANK_H = 8;
constexpr int PLATE_W = 24;
constexpr int PLATE_H = 28;

struct Art {
    gs::Mipped book[4];
    gs::Mipped plate[4];
    gs::Mipped cart;
    gs::Mipped plank;
    gs::Mipped rail;
    gs::Mipped residents;
    gs::Mipped caseBack;
    gs::Mipped bracket;
    gs::Mipped lamp;
    gs::Mipped logo;
    gs::Mipped come;
    gs::Mipped clear;
    gs::Mipped goes;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace shelve
