// S3 FERRY BOX sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace ferrybox {

// Painted hull extents on the 96px sheet, and the matching world size.
constexpr float kPaintL = 37.f;
constexpr float kPaintB = 15.f;
constexpr float kHalfL = 11.f;
constexpr float kHalfB = kHalfL * kPaintB / kPaintL;
constexpr int kHullPx = 96;
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
    PAL_LIGHT = 11,
};

struct Art {
    gs::Mipped hull[16];
    gs::Mipped pad;
    gs::Mipped post;
    gs::Mipped hbar;
    gs::Mipped vbar;
    gs::Mipped quay;
    gs::Mipped shed;
    gs::Mipped light;
    gs::Mipped foam;
    gs::Mipped smoke;
    gs::Mipped chev;
    gs::Mipped gull[2];
    gs::Mipped title;
    gs::Mipped boxWord;
    gs::Mipped stopped;
    gs::Mipped outside;
    gs::Mipped late;
    gs::Mipped paused;
    gs::Mipped north;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace ferrybox
