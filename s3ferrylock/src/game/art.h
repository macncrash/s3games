// S3 FERRY LOCK sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace ferrylock {

// Hull paint: bow is +local Y, starboard is +local X. Collision uses the same ratio.
constexpr float kHalfL = 7.2f;
constexpr float kPaintBow = 34.f;
constexpr float kPaintBeam = 15.f;
constexpr int kHullPx = 96;
constexpr float kHalfB = kHalfL * kPaintBeam / kPaintBow;
constexpr float kSpriteWorld = kHalfL * float(kHullPx) / kPaintBow;

constexpr int kLeafFrames = 48;
constexpr int kLeafPx = 64;
constexpr float kLeafPaint = 52.f;

enum Pal : int {
    PAL_HUD = 0,
    PAL_FERRY = 1,
    PAL_BANK = 2,
    PAL_GATE = 3,
    PAL_FOAM = 4,
    PAL_BUOY = 5,
    PAL_GULL = 6,
    PAL_WIN = 7,
    PAL_ALERT = 8,
    PAL_BANNER = 9,
    PAL_DIM = 10,
    PAL_HOUSE = 11,
    PAL_MARK = 12,
};

struct Art {
    gs::Mipped hull[16];
    gs::Mipped leaf[kLeafFrames];
    gs::Mipped wall;
    gs::Mipped shore;
    gs::Mipped sill;
    gs::Mipped house;
    gs::Mipped lamp;
    gs::Mipped bollard;
    gs::Mipped post;
    gs::Mipped buoy[2];
    gs::Mipped dash;
    gs::Mipped foam;
    gs::Mipped gull[2];
    gs::Mipped title;
    gs::Mipped endSign;
    gs::Mipped clearWord;
    gs::Mipped scraped;
    gs::Mipped missed;
    gs::Mipped paused;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace ferrylock
