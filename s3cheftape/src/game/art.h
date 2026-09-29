// S3 CHEFTAPE pictures. Drawn into VRAM at boot. No asset files.
#pragma once

#include "console/gfx.h"
#include "console/vdp.h"

namespace cheftape {

enum Pal {
    PAL_TEXT = 0,
    PAL_KITCHEN = 1,
    PAL_SOUP = 2,
    PAL_STEAK = 3,
    PAL_CAKE = 4,
    PAL_GRAVY = 5,
    PAL_GOLD = 6,
    PAL_PAPER = 7,
    PAL_STEEL = 8,
    PAL_ALERT = 9,
    PAL_FIRE = 10
};

constexpr int kDishes = 4;
constexpr int kTapeN = 3;

struct Dish {
    const char* name;
    int pay;
    bool decoy;
    int pal;
    float pitch;
};

// The tape is SOUP, STEAK, CAKE. GRAVY pays the same as STEAK and stays out.
constexpr Dish kDish[kDishes] = {
    {"SOUP", 2, false, PAL_SOUP, 392.00f},
    {"STEAK", 6, false, PAL_STEAK, 261.63f},
    {"CAKE", 4, false, PAL_CAKE, 523.25f},
    {"GRAVY", 6, true, PAL_GRAVY, 146.83f},
};

constexpr int kTape[kTapeN] = {0, 1, 2};

struct Art {
    int font[96] = {};
    gs::Mipped plate[kDishes];
    gs::Mipped chef;
    gs::Mipped reel;
    gs::Mipped ticket;
    gs::Mipped pan;
    gs::Mipped solid;
    gs::Mipped word;
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace cheftape
