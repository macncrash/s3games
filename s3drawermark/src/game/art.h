// S3 DRAWERMARK pictures. Everything is drawn at boot. There are no asset files.
#pragma once
#include <cstdint>

#include "console/gfx.h"
#include "console/vdp.h"

namespace drawermark {

enum class Kind : uint8_t { Gold, One, Button, Count };

constexpr float kAsideY = 118.f;
constexpr float kWellX = 160.f;
constexpr float kWellY = 176.f;
constexpr float kCoverTop = 142.f;
constexpr int kMarkCents = 500;

enum Pal {
    PAL_TEXT = 0,
    PAL_AMBER = 1,
    PAL_RED = 2,
    PAL_GREEN = 3,
    PAL_SHOP = 4,
    PAL_BILL = 5,
    PAL_GOLD = 6,
    PAL_JUNK = 7,
    PAL_WOOD = 8,
    PAL_INK = 9,
    PAL_SHADE = 10
};

struct Art {
    gs::Mipped piece[int(Kind::Count)];
    gs::Mipped caret;
    gs::Mipped cover;
    gs::Mipped solid;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace drawermark
