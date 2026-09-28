// S3 QUARRY PURSUIT pictures. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace quarrypurs {

enum Pal {
    PAL_HUD = 0,
    PAL_ALERT = 1,
    PAL_GOOD = 2,
    PAL_GOLD = 3,
    PAL_YOU = 4,
    PAL_DUMP = 5,
    PAL_HOE = 6,
    PAL_CRUSH = 7,
    PAL_FX = 8,
    PAL_ROCK = 9,
    PAL_DUST = 10,
    PAL_PROP = 11
};

// Three haul benches cut into the face. Feet sit on the bottom of the row.
inline constexpr int kBenchRow[3] = {10, 16, 22};
inline constexpr float kBenchY[3] = {10 * 8 + 7, 16 * 8 + 7, 22 * 8 + 7};
inline constexpr float kMinX = 36.f;
inline constexpr float kMaxX = 292.f;

struct Art {
    gs::Mipped loader[2];
    gs::Mipped dumper[2];
    gs::Mipped hoe[2];
    gs::Mipped crush[2];
    gs::Mipped rock, puff, spark, shadow, cone, boulder, lamp;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace quarrypurs
