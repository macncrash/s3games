// S3 DRUMCHIME pictures. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace drumchime {

constexpr int kPads = 5;
constexpr int kGold = 1;
constexpr int kCream = 2;
constexpr int kSticks = 3;
constexpr int kFpc = 6;
constexpr int kGraceSec = 24;
constexpr int kHourSec = 12 * 3600;
constexpr int kStartSec = kHourSec - 40;

inline bool goldPad(int i) { return i == kGold; }
inline bool creamPad(int i) { return i == kCream; }

enum Pal {
    PAL_HUD = 0,
    PAL_GOLD = 1,
    PAL_DIM = 2,
    PAL_BAD = 3,
    PAL_SHELL = 4,
    PAL_HEAD = 5,
    PAL_LIT = 6,
    PAL_CREAM = 7,
    PAL_BELL = 8,
    PAL_STICK = 9,
    PAL_STAGE = 10,
    PAL_FACE = 11
};

struct Art {
    gs::Image shell;
    gs::Image bass;
    gs::Image headGold;
    gs::Image headCream;
    gs::Image headWood;
    gs::Image cym;
    gs::Image stick;
    gs::Image bell;
    gs::Image face;
    gs::Image hand;
    gs::Image player;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace drumchime
