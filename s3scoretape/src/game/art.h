// S3 SCORETAPE pictures. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace scoretape {

constexpr int kMarks = 4;
constexpr int kTapeN = 3;
constexpr int kDieAt = 78;
constexpr int kSweetLo = 42;
constexpr int kSweetHi = 54;

enum Pal {
    PAL_HUD = 0,
    PAL_GOLD = 1,
    PAL_DIM = 2,
    PAL_BAD = 3,
    PAL_PAPER = 4,
    PAL_INK = 5,
    PAL_WOOD = 6,
    PAL_QUAVER = 7,
    PAL_MINIM = 8,
    PAL_BREVE = 9,
    PAL_REST = 10
};

// The tape is QUAVER, MINIM, BREVE. REST pays the same as MINIM and stays out.
struct Mark {
    const char* name;
    int pay;
    bool decoy;
    int pal;
};

constexpr Mark kMark[kMarks] = {
    {"QUAVER", 4, false, PAL_QUAVER},
    {"MINIM", 7, false, PAL_MINIM},
    {"BREVE", 5, false, PAL_BREVE},
    {"REST", 7, true, PAL_REST},
};

constexpr int kTape[kTapeN] = {0, 1, 2};

struct Art {
    gs::Image sheet;
    gs::Image quaver;
    gs::Image minim;
    gs::Image breve;
    gs::Image rest;
    gs::Image drawer;
    gs::Image lamp;
    gs::Image pen;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace scoretape
