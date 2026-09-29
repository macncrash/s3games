// S3 PALISADE DAWN sprites. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace pdawn {

enum Pal {
    PAL_HUD = 0,
    PAL_GROUND = 1,
    PAL_TIMBER = 2,
    PAL_YOU = 3,
    PAL_BASKET = 4,
    PAL_FIRE = 5,
    PAL_MOON = 6,
    PAL_STAR = 7,
    PAL_OIL = 8,
    PAL_GOLD = 9,
    PAL_ALERT = 10,
    PAL_OK = 11,
    PAL_WIND = 12
};

constexpr int kFlares = 5;
constexpr float FLARE_X[kFlares] = {40.f, 100.f, 160.f, 220.f, 280.f};
constexpr float WALK_Y = 152.f;

struct Art {
    gs::Mipped stake;
    gs::Mipped flame[2];
    gs::Mipped sentry[2];
    gs::Mipped flask;
    gs::Mipped moon;
    gs::Mipped sun;
    gs::Mipped star;
    gs::Mipped gust;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace pdawn
