// Pictures for a short shelve. Drawn into the VDP at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace shelveseven {

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
    PAL_MARK = 10
};

inline int bookPal(int row) { return PAL_A + (row & 3); }

constexpr float ROW_Y0 = 52.f;
constexpr float ROW_DY = 40.f;
constexpr float CART_X = 36.f;
constexpr float HAND_X = 88.f;
constexpr float PLATE_X = 118.f;
constexpr float SLOT_X0 = 168.f;
constexpr float SLOT_DX = 16.f;
constexpr float BOOK_DH = 28.f;

struct Art {
    gs::Mipped book[4];
    gs::Mipped cart;
    gs::Mipped plank;
    gs::Mipped plate;
    gs::Mipped seven;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace shelveseven
