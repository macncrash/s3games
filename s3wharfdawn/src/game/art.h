// S3 WHARF DAWN sprites. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace wharfdawn {

enum Pal {
    PAL_HUD = 0,
    PAL_WATER = 1,
    PAL_PIER = 2,
    PAL_YOU = 3,
    PAL_LAMP = 4,
    PAL_FIRE = 5,
    PAL_MOON = 6,
    PAL_STAR = 7,
    PAL_BOAT = 8,
    PAL_GOLD = 9,
    PAL_ALERT = 10,
    PAL_OK = 11,
    PAL_SWELL = 12
};

constexpr int kFlares = 4;
constexpr float FLARE_X[kFlares] = {48.f, 118.f, 202.f, 272.f};
constexpr float FLARE_Y = 148.f;

struct Art {
    gs::Mipped pier;
    gs::Mipped shed;
    gs::Mipped post;
    gs::Mipped lamp;
    gs::Mipped flame[2];
    gs::Mipped keeper[2];
    gs::Mipped boat;
    gs::Mipped gull;
    gs::Mipped moon;
    gs::Mipped sun;
    gs::Mipped star;
    gs::Mipped spray;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace wharfdawn
