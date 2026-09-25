// S3 KEYS pictures. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace keys {

constexpr int kLanes = 4;
constexpr int kLaneX[kLanes] = {80, 128, 176, 224};
constexpr int kHitY = 158;
constexpr float kTravel = 122.f;
constexpr int kLamps = 5;
constexpr int kLampX[kLamps] = {130, 146, 162, 178, 194};

enum Pal {
    PAL_WHITE = 0,
    PAL_N0 = 1,
    PAL_N1 = 2,
    PAL_N2 = 3,
    PAL_N3 = 4,
    PAL_KEY = 5,
    PAL_KEYLIT = 6,
    PAL_FX = 7,
    PAL_LAMP = 8,
    PAL_DIM = 9,
    PAL_STAGE = 10,
    PAL_GOLD = 11,
    PAL_BAD = 12,
    PAL_GOOD = 13
};

struct Art {
    gs::Mipped note;
    gs::Mipped key[kLanes];
    gs::Mipped ring;
    gs::Mipped lamp;
    gs::Mipped lantern;
    gs::Mipped flash;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace keys
