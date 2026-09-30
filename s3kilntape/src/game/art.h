// S3 KILNTAPE pictures. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace kilntape {

constexpr int kWares = 4;
constexpr int kTapeN = 3;
constexpr int kDieAt = 72;
constexpr int kSweetLo = 34;
constexpr int kSweetHi = 46;

enum Pal {
    PAL_HUD = 0,
    PAL_GOLD = 1,
    PAL_DIM = 2,
    PAL_BAD = 3,
    PAL_BRICK = 4,
    PAL_GLAZE = 5,
    PAL_BOWL = 6,
    PAL_CONE = 7,
    PAL_BISQUE = 8,
    PAL_CLAY = 9
};

// The tape is GLAZE, BOWL, CONE. BISQUE pays the same as BOWL and stays out.
struct Ware {
    const char* name;
    int pay;
    bool decoy;
    int pal;
};

constexpr Ware kWare[kWares] = {
    {"GLAZE", 5, false, PAL_GLAZE},
    {"BOWL", 7, false, PAL_BOWL},
    {"CONE", 4, false, PAL_CONE},
    {"BISQUE", 7, true, PAL_BISQUE},
};

constexpr int kTape[kTapeN] = {0, 1, 2};

struct Art {
    gs::Image kiln;
    gs::Image door;
    gs::Image glaze;
    gs::Image bowl;
    gs::Image cone;
    gs::Image bisque;
    gs::Image drawer;
    gs::Image flame;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace kilntape
