// S3 DEPOT PURSUIT pictures. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace depotpurs {

enum Pal {
    PAL_HUD = 0,
    PAL_ALERT = 1,
    PAL_GOOD = 2,
    PAL_GOLD = 3,
    PAL_YOU = 4,
    PAL_DRAY = 5,
    PAL_CRANE = 6,
    PAL_LOCO = 7,
    PAL_FX = 8,
    PAL_YARD = 9,
    PAL_HOUSE = 10,
    PAL_PROP = 11,
    PAL_NIGHT = 12
};

// Three stall roads in the goods yard. Feet stand on the bottom of a rail row.
inline constexpr int kTrackRow[3] = {12, 16, 20};
inline constexpr float kTrackY[3] = {12 * 8 + 7, 16 * 8 + 7, 20 * 8 + 7};
inline constexpr float kMinX = 116.f;
inline constexpr float kMaxX = 284.f;

struct Art {
    gs::Mipped shunter[2];
    gs::Mipped dray[2];
    gs::Mipped crane[2];
    gs::Mipped loco[2];
    gs::Mipped flame[2];
    gs::Mipped shoe, bolt, puff, spark, pip, shadow;
    gs::Mipped buffer, post, barrel, crate, moon, cloud;
    gs::Mipped sign;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace depotpurs
