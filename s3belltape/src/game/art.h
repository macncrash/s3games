// S3 BELLTAPE pictures. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace belltape {

constexpr int kNotes = 4;
constexpr int kTapeN = 3;
constexpr int kDieAt = 78;
constexpr int kSweetLo = 40;
constexpr int kSweetHi = 54;

enum Pal {
    PAL_HUD = 0,
    PAL_GOLD = 1,
    PAL_DIM = 2,
    PAL_BAD = 3,
    PAL_BELL = 4,
    PAL_ROPE = 5,
    PAL_PEAL = 6,
    PAL_KNOCK = 7,
    PAL_TOLL = 8,
    PAL_CLANG = 9,
    PAL_WOOD = 10
};

// The tape is PEAL, KNOCK, TOLL. CLANG pays like KNOCK and stays out.
struct Note {
    const char* name;
    int pay;
    bool decoy;
    int pal;
};

constexpr Note kNote[kNotes] = {
    {"PEAL", 6, false, PAL_PEAL},
    {"KNOCK", 4, false, PAL_KNOCK},
    {"TOLL", 5, false, PAL_TOLL},
    {"CLANG", 4, true, PAL_CLANG},
};

constexpr int kTape[kTapeN] = {0, 1, 2};

struct Art {
    gs::Image belfry;
    gs::Image bell;
    gs::Image clapper;
    gs::Image rope;
    gs::Image peal;
    gs::Image knock;
    gs::Image toll;
    gs::Image clang;
    gs::Image drawer;
    gs::Image lamp;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace belltape
