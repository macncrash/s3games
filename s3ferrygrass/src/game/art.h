// S3 FERRY GRASS sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace ferrygrass {

constexpr float kPaintL = 34.f;
constexpr float kPaintB = 13.f;
constexpr float kHalfL = 8.2f;
constexpr float kHalfB = kHalfL * kPaintB / kPaintL;
constexpr int kHullPx = 80;
constexpr float kSpriteWorld = float(kHullPx) * kHalfL / kPaintL;

enum Pal : int {
    PAL_HUD = 0,
    PAL_FERRY = 1,
    PAL_POST = 2,
    PAL_SHORE = 3,
    PAL_FOAM = 4,
    PAL_GULL = 5,
    PAL_MARK = 6,
    PAL_WIN = 7,
    PAL_ALERT = 8,
    PAL_BANNER = 9,
    PAL_DIM = 10,
    PAL_RIVAL = 11,
};

struct Art {
    gs::Mipped hull[8];
    gs::Mipped tuft;
    gs::Mipped post;
    gs::Mipped hbar;
    gs::Mipped vbar;
    gs::Mipped shed;
    gs::Mipped foam;
    gs::Mipped smoke;
    gs::Mipped gull[2];
    gs::Mipped title;
    gs::Mipped grassWord;
    gs::Mipped stopped;
    gs::Mipped missed;
    gs::Mipped late;
    gs::Mipped paused;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace ferrygrass
