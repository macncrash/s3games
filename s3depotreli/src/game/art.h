// S3 DEPOT RELIEF pictures. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace depot {

enum Pal {
    PAL_HUD = 0,
    PAL_AMBER = 1,
    PAL_RED = 2,
    PAL_GREEN = 3,
    PAL_CLERK = 4,
    PAL_LOOT = 5,
    PAL_DRAY = 6,
    PAL_WAGON = 7,
    PAL_WOOD = 8,
    PAL_BELL = 9,
    PAL_HOUSE = 10,
    PAL_NIGHT = 11,
    PAL_YARD = 12,
    PAL_LOCO = 13,
    PAL_FX = 14
};

// Side elevation of the headhouse. Feet stand on the bottom pixel of a rail row.
inline constexpr int kTrackRow[3] = {14, 18, 22};
inline constexpr int kTrackY[3] = {14 * 8 + 7, 18 * 8 + 7, 22 * 8 + 7};
inline constexpr float kPlatformY = 88.f;
inline constexpr float kBreachX = 76.f;
inline constexpr float kSpawnX = 326.f;
inline constexpr float kStandX = 132.f;

struct Art {
    gs::Mipped clerk[2];
    gs::Mipped runner[2];
    gs::Mipped dray[2];
    gs::Mipped wagon[2];
    gs::Mipped loco;
    gs::Mipped bell, rope, lantern, signal, buffer, crate, barrel;
    gs::Mipped lamp, glint, dust, shadow, cloud, moon, star, sign;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace depot
