// S3 DRUM GOLD pictures. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace drumgold {

constexpr int kPads = 8;
constexpr int kLine = 60;
constexpr int kGoldFace = 10;
constexpr int kCreamFace = 9;
constexpr int kGoldNeed = 3;

// Pads: 0 bass, 1 gold snare, 2 cream rack, 3 floor, 4 gold ride, 5 cream hat, 6 gold crash, 7 wood tom.
inline bool goldPad(int i) { return i == 1 || i == 4 || i == 6; }
inline bool creamPad(int i) { return i == 2 || i == 5; }

enum Pal {
    PAL_HUD = 0,
    PAL_GOLD = 1,
    PAL_DIM = 2,
    PAL_BAD = 3,
    PAL_SHELL = 4,
    PAL_HEAD = 5,
    PAL_LIT = 6,
    PAL_CREAM = 7,
    PAL_CYM = 8,
    PAL_STICK = 9,
    PAL_STAGE = 10
};

struct Art {
    gs::Image shell;
    gs::Image headGold;
    gs::Image headCream;
    gs::Image headWood;
    gs::Image bass;
    gs::Image cymGold;
    gs::Image cymCream;
    gs::Image cymWood;
    gs::Image stick;
    gs::Image stand;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace drumgold
