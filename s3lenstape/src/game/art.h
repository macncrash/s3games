// S3 LENSTAPE pictures. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace lenstape {

constexpr int kGlass = 4;
constexpr int kTapeN = 3;
constexpr int kPassAt = 90;
constexpr int kGateLo = 40;
constexpr int kGateHi = 54;

enum Pal {
    PAL_HUD = 0,
    PAL_GOLD = 1,
    PAL_DIM = 2,
    PAL_BAD = 3,
    PAL_BENCH = 4,
    PAL_CROWN = 5,
    PAL_FLINT = 6,
    PAL_MENISCUS = 7,
    PAL_PRISM = 8,
    PAL_PAPER = 9
};

// The tape is CROWN, FLINT, MENISCUS. PRISM pays like FLINT and stays out.
struct Glass {
    const char* name;
    int pay;
    bool decoy;
    int pal;
};

constexpr Glass kGlassSpec[kGlass] = {
    {"CROWN", 4, false, PAL_CROWN},
    {"FLINT", 7, false, PAL_FLINT},
    {"MENISCUS", 6, false, PAL_MENISCUS},
    {"PRISM", 7, true, PAL_PRISM},
};

constexpr int kTape[kTapeN] = {0, 1, 2};

struct Art {
    gs::Image bench;
    gs::Image gate;
    gs::Image crown;
    gs::Image flint;
    gs::Image meniscus;
    gs::Image prism;
    gs::Image tray;
    gs::Image lamp;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace lenstape
