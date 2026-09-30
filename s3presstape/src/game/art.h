// S3 PRESSTAPE pictures. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace presstape {

constexpr int kBlanks = 5;
constexpr int kTapeN = 3;
constexpr int kDwell = 70;
constexpr int kStroke = 36;

enum Pal {
    PAL_HUD = 0,
    PAL_GOLD = 1,
    PAL_DIM = 2,
    PAL_BAD = 3,
    PAL_STEEL = 4,
    PAL_OIL = 5,
    PAL_RIB = 6,
    PAL_WEB = 7,
    PAL_CAP = 8,
    PAL_SCRAP = 9,
    PAL_DOOR = 10
};

// The tape is RIB, WEB, CAP. FLASH pays like WEB and must stay out of the drawer.
// SHIM is scrap and never belongs on the tape.
struct Blank {
    const char* name;
    int pay;
    bool reject;
    int pal;
};

constexpr Blank kBlank[kBlanks] = {
    {"RIB", 6, false, PAL_RIB},
    {"WEB", 4, false, PAL_WEB},
    {"CAP", 8, false, PAL_CAP},
    {"FLASH", 4, true, PAL_SCRAP},
    {"SHIM", 1, true, PAL_SCRAP},
};

constexpr int kTape[kTapeN] = {0, 1, 2};

struct Art {
    gs::Image frame;
    gs::Image ram;
    gs::Image bed;
    gs::Image rib;
    gs::Image web;
    gs::Image cap;
    gs::Image flash;
    gs::Image shim;
    gs::Image drawer;
    gs::Image door;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace presstape
