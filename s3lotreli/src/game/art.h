// S3 LOT RELIEF pictures. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace lot {

enum Pal {
    PAL_HUD = 0,
    PAL_AMBER = 1,
    PAL_RED = 2,
    PAL_GREEN = 3,
    PAL_WATCH = 4,
    PAL_SKATE = 5,
    PAL_SEDAN = 6,
    PAL_VAN = 7,
    PAL_CHAIN = 8,
    PAL_BELL = 9,
    PAL_WALL = 10,
    PAL_NIGHT = 11,
    PAL_LOT = 12,
    PAL_RELIEF = 13,
    PAL_FX = 14
};

// Three stall rows. Feet stand on the bottom pixel of a row.
inline constexpr int kRowCell[3] = {14, 18, 22};
inline constexpr int kRowY[3] = {14 * 8 + 7, 18 * 8 + 7, 22 * 8 + 7};
inline constexpr float kGateX = 76.f;
inline constexpr float kSpawnX = 326.f;
inline constexpr float kStandX = 132.f;

struct Art {
    gs::Mipped watch[2];
    gs::Mipped skate[2];
    gs::Mipped sedan[2];
    gs::Mipped van[2];
    gs::Mipped relief;
    gs::Mipped bell, rope, lampPost, cone, bollard, crate, drum;
    gs::Mipped flare, glint, dust, shadow, cloud, moon, star, sign;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace lot
