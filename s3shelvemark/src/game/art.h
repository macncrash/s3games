// S3 SHELVE MARK pictures. Drawn into VDP RAM at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace shelvemark {

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

constexpr float ROW_Y0 = 50.f;
constexpr float ROW_DY = 42.f;
constexpr float CART_X = 46.f;
constexpr float HAND_X = 100.f;
constexpr float PLATE_X = 136.f;
constexpr float SLOT_X0 = 214.f;
constexpr float SLOT_DX = 22.f;
constexpr float BOOK_DH = 30.f;
constexpr int BOOK_W = 16;
constexpr int BOOK_H = 30;

struct Art {
    gs::Mipped book[4];
    gs::Mipped plate[4];
    gs::Mipped cart;
    gs::Mipped shelf;
    gs::Mipped post;
    gs::Mipped mark;
    gs::Mipped lamp;
    gs::Mipped logo;
    gs::Mipped backBan;
    gs::Mipped heldBan;
    gs::Mipped doneBan;
    gs::Mipped lostBan;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace shelvemark
