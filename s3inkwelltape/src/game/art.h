// S3 INKWELLTAPE pictures. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace inkwelltape {

constexpr int kInks = 4;
constexpr int kTapeN = 3;
constexpr int kDieAt = 72;
constexpr int kSweetLo = 40;
constexpr int kSweetHi = 50;

enum Pal {
    PAL_HUD = 0,
    PAL_GOLD = 1,
    PAL_DIM = 2,
    PAL_BAD = 3,
    PAL_DESK = 4,
    PAL_QUILL = 5,
    PAL_GALL = 6,
    PAL_SEPIA = 7,
    PAL_LAMP = 8,
    PAL_WASH = 9,
    PAL_PAPER = 10,
    PAL_DRAWER = 11
};

// The tape is GALL, SEPIA, LAMP. WASH pays like SEPIA and stays out of the drawer.
struct Ink {
    const char* name;
    int pay;
    bool decoy;
    int pal;
};

constexpr Ink kInk[kInks] = {
    {"GALL", 5, false, PAL_GALL},
    {"SEPIA", 8, false, PAL_SEPIA},
    {"LAMP", 3, false, PAL_LAMP},
    {"WASH", 8, true, PAL_WASH},
};

constexpr int kTape[kTapeN] = {0, 1, 2};

struct Art {
    gs::Image desk;
    gs::Image well;
    gs::Image pool;
    gs::Image quill;
    gs::Image bead;
    gs::Image drawer;
    gs::Image tape;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace inkwelltape
