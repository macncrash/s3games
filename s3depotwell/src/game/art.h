// S3 DEPOT WELL sprites. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace depotwell {

enum Pal {
    PAL_TEXT = 0,
    PAL_GOLD = 1,
    PAL_ALERT = 2,
    PAL_GOOD = 3,
    PAL_YARD = 4,
    PAL_STONE = 5,
    PAL_ENGINE = 6,
    PAL_BOX = 7,
    PAL_FLAT = 8,
    PAL_TANK = 9,
    PAL_FX = 10,
    PAL_SKY = 11
};

// Coupler mark and the stone lip of the well, in screen pixels.
constexpr float kMarkX = 156.f;
constexpr float kImpactX = 238.f;
constexpr float kSpawnX = -40.f;
constexpr float kWellX = 286.f;
constexpr float kWellFoot = 200.f;

// Three sidings. Feet sit on the rail.
inline float laneFoot(int lane) { return 108.f + 38.f * float(lane); }

struct Art {
    gs::Mipped well;
    gs::Mipped rubble;
    gs::Mipped crack;
    gs::Mipped bucket;
    gs::Mipped shunter[2];
    gs::Mipped box[2];
    gs::Mipped flat[2];
    gs::Mipped tank[2];
    gs::Mipped buffer;
    gs::Mipped puff;
    gs::Mipped spark;
    gs::Mipped shadow;
    gs::Mipped moon;
    gs::Mipped star;
    gs::Mipped lamp;
    gs::Mipped chain;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace depotwell
