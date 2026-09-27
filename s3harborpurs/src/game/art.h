// S3 HARBOR PURSUIT pictures. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace harborpurs {

enum Pal {
    PAL_HUD = 0,
    PAL_ALERT = 1,
    PAL_GOOD = 2,
    PAL_GOLD = 3,
    PAL_YOU = 4,
    PAL_TUG = 5,
    PAL_FERRY = 6,
    PAL_CUT = 7,
    PAL_FX = 8,
    PAL_QUAY = 9,
    PAL_HOUSE = 10,
    PAL_WATER = 11,
    PAL_LAMP = 12
};

// Three fairway channels. Hulls sit on the waterline of each row.
inline constexpr int kLaneRow[3] = {13, 17, 21};
inline constexpr float kLaneY[3] = {13 * 8 + 6, 17 * 8 + 6, 21 * 8 + 6};
inline constexpr float kMinX = 108.f;
inline constexpr float kMaxX = 300.f;

struct Art {
    gs::Mipped pilot[2];
    gs::Mipped tug[2];
    gs::Mipped ferry[2];
    gs::Mipped cutter[2];
    gs::Mipped wake, bolt, spark, buoy, gull, lamp;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace harborpurs
