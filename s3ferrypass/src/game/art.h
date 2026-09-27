// S3 FERRY PASS sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace ferrypass {

// Hull paint: bow is +local Y in the world, starboard is +local X.
constexpr float kHalfL = 6.4f;
constexpr float kPaintBow = 28.f;
constexpr float kPaintBeam = 12.f;
constexpr int kHullPx = 80;
constexpr float kHalfB = kHalfL * kPaintBeam / kPaintBow;

enum Pal : int {
    PAL_HUD = 0,
    PAL_FERRY = 1,
    PAL_CLIFF = 2,
    PAL_FOAM = 3,
    PAL_TAPE = 4,
    PAL_STORM = 5,
    PAL_GULL = 6,
    PAL_WIN = 7,
    PAL_ALERT = 8,
    PAL_BANNER = 9,
    PAL_DIM = 10,
    PAL_HUT = 11,
    PAL_LIGHT = 12,
};

struct Art {
    gs::Mipped hull[8];
    gs::Mipped cliff;
    gs::Mipped scrub;
    gs::Mipped rock;
    gs::Mipped foam;
    gs::Mipped post;
    gs::Mipped tape;
    gs::Mipped hut;
    gs::Mipped lamp;
    gs::Mipped gull[2];
    gs::Mipped title;
    gs::Mipped clearWord;
    gs::Mipped missed;
    gs::Mipped paused;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace ferrypass
