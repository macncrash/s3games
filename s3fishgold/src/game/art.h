// S3 FISH GOLD pictures. Drawn at boot. No asset files.
#pragma once

#include "console/gfx.h"
#include "console/vdp.h"

namespace fishgold {

constexpr int kLine = 40;
constexpr int kCasts = 8;

enum Pal {
    PAL_GOLD = 0,
    PAL_CREAM = 1,
    PAL_SILVER = 2,
    PAL_MINNOW = 3,
    PAL_BOAT = 4,
    PAL_LURE = 5,
    PAL_REED = 6,
    PAL_SUN = 7,
    PAL_HUD = 8,
    PAL_ALERT = 9,
    PAL_INK = 10
};

enum KindId { KIND_GOLD = 0, KIND_CREAM = 1, KIND_SILVER = 2, KIND_MINNOW = 3 };

struct Kind {
    const char* name;
    int face;
    int points;
    bool gold;
    bool cream;
};

// Only the gold counts double. Cream scores its face and cannot buy the line.
inline Kind kindOf(int id) {
    switch (id) {
    case KIND_GOLD: return {"GOLD", 10, 20, true, false};
    case KIND_CREAM: return {"CREAM", 9, 9, false, true};
    case KIND_SILVER: return {"SILVER", 4, 4, false, false};
    default: return {"MINNOW", 2, 2, false, false};
    }
}

struct Art {
    gs::Image fish[4];
    gs::Image boat;
    gs::Image angler;
    gs::Image lure;
    gs::Image reed;
    gs::Image sun;
    gs::Image splash;
    gs::Image glyph[96];
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace fishgold
