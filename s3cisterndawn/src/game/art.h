// S3 CISTERN DAWN sprites. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace cdawn {

enum Pal {
    PAL_HUD = 0,
    PAL_YARD = 1,
    PAL_STONE = 2,
    PAL_YOU = 3,
    PAL_POST = 4,
    PAL_FIRE = 5,
    PAL_MOON = 6,
    PAL_STAR = 7,
    PAL_WATER = 8,
    PAL_GOLD = 9,
    PAL_ALERT = 10,
    PAL_OK = 11,
    PAL_RAIN = 12
};

constexpr int kFlares = 5;
constexpr float CX = 160.f;
constexpr float CY = 128.f;
constexpr float RX = 96.f;
constexpr float RY = 54.f;

struct Art {
    gs::Mipped cistern;
    gs::Mipped post;
    gs::Mipped flame[2];
    gs::Mipped keeper[2];
    gs::Mipped moon;
    gs::Mipped sun;
    gs::Mipped star;
    gs::Mipped drop;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace cdawn
