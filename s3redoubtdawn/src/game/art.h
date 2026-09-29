// S3 REDOUBT DAWN sprites. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace rdawn {

enum Pal {
    PAL_HUD = 0,
    PAL_EARTH = 1,
    PAL_WALL = 2,
    PAL_YOU = 3,
    PAL_IRON = 4,
    PAL_FIRE = 5,
    PAL_MOON = 6,
    PAL_STAR = 7,
    PAL_FLAG = 8,
    PAL_GOLD = 9,
    PAL_ALERT = 10,
    PAL_OK = 11,
    PAL_RAIN = 12,
    PAL_OIL = 13
};

constexpr int kFlares = 3;
constexpr float FLARE_X[kFlares] = {90.f, 160.f, 230.f};
constexpr float FLARE_Y = 138.f;
constexpr float WALK_Y = 168.f;
constexpr float BARREL_X = 125.f;

struct Art {
    gs::Mipped redoubt;
    gs::Mipped ditch;
    gs::Mipped brazier;
    gs::Mipped flame[2];
    gs::Mipped watch[2];
    gs::Mipped barrel;
    gs::Mipped flag;
    gs::Mipped moon;
    gs::Mipped sun;
    gs::Mipped star;
    gs::Mipped drop;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace rdawn
