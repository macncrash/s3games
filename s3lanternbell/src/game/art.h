// Pictures drawn at boot. Nothing is loaded from a file.
#pragma once

#include "console/gfx.h"
#include "console/vdp.h"

namespace lanternbell {

constexpr int kLamps = 3;
constexpr int kBell = 1;
constexpr int kOrderN = 3;
constexpr int kOrder[kOrderN] = {0, 2, kBell};
constexpr float kPostX[kLamps] = {72.f, 160.f, 248.f};
constexpr float kGround = 188.f;

enum Pal {
    PAL_YARD = 0,
    PAL_LAMP = 1,
    PAL_FLAME = 2,
    PAL_BELL = 3,
    PAL_HAND = 4,
    PAL_INK = 5,
    PAL_GOLD = 6,
    PAL_GREEN = 7,
    PAL_ALERT = 8,
    PAL_TITLE = 9,
    PAL_LEAVE = 10,
    PAL_MOON = 11
};

struct Art {
    gs::Image post;
    gs::Image lamp;
    gs::Image flame;
    gs::Image bell;
    gs::Image clapper;
    gs::Image carry;
    gs::Image moon;
    gs::Image star;
    gs::Image title;
    gs::Image leave;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace lanternbell
