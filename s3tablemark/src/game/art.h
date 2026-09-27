#pragma once

#include "console/gfx.h"
#include "console/vdp.h"

namespace tablemark {

constexpr float kMarkX = 160.f;
constexpr float kMarkY = 66.f;
constexpr float kMarkR = 11.f;
constexpr float kPuckR = 5.f;
constexpr float kMalletR = 12.f;
constexpr float kLeft = 48.f;
constexpr float kRight = 272.f;
constexpr float kTop = 46.f;
constexpr float kBot = 190.f;
constexpr float kDecel = 96.f;

enum Pal {
    PAL_TABLE = 0,
    PAL_PUCK = 1,
    PAL_YOU = 2,
    PAL_THEM = 3,
    PAL_MARK = 4,
    PAL_INK = 5,
    PAL_GOLD = 6,
    PAL_TITLE = 7,
    PAL_WIN = 8,
    PAL_HINT = 9
};

struct Art {
    gs::Image table;
    gs::Image puck;
    gs::Image mallet;
    gs::Image title;
    gs::Image win;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace tablemark
