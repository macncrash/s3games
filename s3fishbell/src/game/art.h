// S3 FISHBELL pictures. Drawn at boot. No asset files.
#pragma once

#include "console/gfx.h"
#include "console/vdp.h"

namespace fishbell {

enum Pal {
    PAL_HUD = 0,
    PAL_GOLD = 1,
    PAL_ALERT = 2,
    PAL_FISH = 3,
    PAL_WEED = 4,
    PAL_BELL = 5,
    PAL_ROD = 6,
    PAL_WIN = 7,
    PAL_WATER = 8,
    PAL_SKY = 9
};

// The keeper crosses this x. The weed crosses it earlier.
constexpr float kHookHome = 196.f;
constexpr float kFishY = 156.f;
constexpr float kWeedY = 150.f;
constexpr float kFishX0 = -36.f;
constexpr float kWeedX0 = -28.f;
constexpr float kFishSpd = 72.f;
constexpr float kWeedSpd = 118.f;
constexpr float kFishDelay = 2.15f;
constexpr float kStrike = 9.f;
constexpr float kSnag = 13.f;

struct Art {
    int font[96] = {};
    gs::Mipped fish;
    gs::Mipped weed;
    gs::Mipped hook;
    gs::Mipped bobber;
    gs::Mipped bell;
    gs::Mipped clapper;
    gs::Mipped yoke;
    gs::Mipped rod;
    gs::Mipped post;
    gs::Mipped ripple;
};

void loadArt(gs::VDP& vdp, Art& art);

}  // namespace fishbell
