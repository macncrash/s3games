// S3 DRAWER pictures. Everything is drawn at boot. There are no asset files.
#pragma once
#include <cstdint>

#include "console/gfx.h"
#include "console/vdp.h"

namespace drawer {

enum class Kind : uint8_t {
    Bill20, Bill10, Bill5, Bill1,
    Quarter, Dime, Nickel, Penny,
    Button, Iou, Token, Clip,
    Count
};

// Shared by the shop painting and the piece layout.
constexpr float kSlotX0 = 28.f;
constexpr float kSlotPitch = 37.f;
constexpr float kAsideY = 126.f;
constexpr float kMessY = 156.f;
constexpr float kDrawerY = 172.f;
constexpr float kCoverTop = 140.f;
constexpr int kSignX = 258;
constexpr int kSignY = 4;
constexpr int kSignW = 56;
constexpr int kSignH = 18;

enum Pal {
    PAL_TEXT = 0,
    PAL_AMBER = 1,
    PAL_RED = 2,
    PAL_GREEN = 3,
    PAL_SHOP = 4,
    PAL_BILL = 5,
    PAL_COIN = 6,
    PAL_JUNK = 7,
    PAL_WOOD = 8,
    PAL_INK = 9,
    PAL_SIGN = 10,
    PAL_SHADE = 11
};

struct Art {
    gs::Mipped piece[int(Kind::Count)];
    gs::Mipped caret;
    gs::Mipped cover;
    gs::Mipped sign;
    gs::Mipped solid;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace drawer
