// S3 PLOW PASS sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace plowpass {

// Blade faces +local Y. Cab sits behind it.
constexpr float kHalfL = 4.6f;
constexpr float kPaintNose = 22.f;
constexpr float kPaintBeam = 16.f;
constexpr int kPlowPx = 64;
constexpr float kHalfB = kHalfL * kPaintBeam / kPaintNose;

enum Pal : int {
    PAL_HUD = 0,
    PAL_PLOW = 1,
    PAL_BANK = 2,
    PAL_PINE = 3,
    PAL_TAPE = 4,
    PAL_STORM = 5,
    PAL_SNOW = 6,
    PAL_WIN = 7,
    PAL_ALERT = 8,
    PAL_BANNER = 9,
    PAL_DIM = 10,
    PAL_SHED = 11,
    PAL_LIGHT = 12,
};

struct Art {
    gs::Mipped plow[8];
    gs::Mipped bank;
    gs::Mipped pine;
    gs::Mipped drift;
    gs::Mipped spray;
    gs::Mipped post;
    gs::Mipped tape;
    gs::Mipped shed;
    gs::Mipped lamp;
    gs::Mipped flake[2];
    gs::Mipped title;
    gs::Mipped clearWord;
    gs::Mipped missed;
    gs::Mipped paused;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace plowpass
