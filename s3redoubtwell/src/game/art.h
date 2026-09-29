// S3 REDOUBT WELL sprites. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace rwell {

enum Pal {
    PAL_HUD = 0,
    PAL_WORK = 1,
    PAL_WELL = 2,
    PAL_YOU = 3,
    PAL_FOE = 4,
    PAL_SAP = 5,
    PAL_RUN = 6,
    PAL_FX = 7,
    PAL_AMBER = 8,
    PAL_ALERT = 9,
    PAL_OK = 10
};

constexpr int LANES = 4;
constexpr float LANE_X[LANES] = {52.f, 116.f, 204.f, 268.f};
constexpr float STEP_Y = 96.f;
constexpr float WELL_X = 160.f;
constexpr float WELL_Y = 58.f;

struct Art {
    gs::Mipped sentry[2];
    gs::Mipped musket[2];
    gs::Mipped sapper[2];
    gs::Mipped runner[2];
    gs::Mipped well;
    gs::Mipped crack;
    gs::Mipped shot;
    gs::Mipped muzzle;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace rwell
