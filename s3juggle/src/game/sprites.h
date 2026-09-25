// S3 JUGGLE sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace juggle {

enum Pal {
    PAL_HUD = 0,
    PAL_GOLD = 1,
    PAL_RED = 2,
    PAL_GREEN = 3,
    PAL_DIM = 4,
    PAL_BODY = 5,
    PAL_BALL0 = 6,
    PAL_BALL1 = 7,
    PAL_BALL2 = 8,
    PAL_PROP = 9,
    PAL_FX = 10
};

// Body bitmap. The anchor pixel is placed on the stage catch line.
constexpr int kBodyW = 176;
constexpr int kBodyH = 156;
constexpr float kAnchorX = 88.f;
constexpr float kAnchorY = 110.f;
constexpr float kGloveL = 24.f;
constexpr float kGloveR = 152.f;
constexpr float kGloveY = 110.f;

struct Art {
    gs::Mipped body;
    gs::Mipped ball[4];
    gs::Mipped ring;
    gs::Mipped star;
    gs::Mipped shadow;
    gs::Mipped curtain;
    gs::Mipped valance;
    gs::Mipped lamp;
    gs::Mipped pool;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace juggle
