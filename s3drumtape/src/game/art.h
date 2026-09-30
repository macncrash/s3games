// S3 DRUMTAPE pictures. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace drumtape {

constexpr int kParts = 4;
constexpr int kTapeN = 3;
constexpr int kDieAt = 72;
constexpr int kSweetLo = 30;
constexpr int kSweetHi = 42;

enum Pal {
    PAL_HUD = 0,
    PAL_GOLD = 1,
    PAL_DIM = 2,
    PAL_BAD = 3,
    PAL_SHELL = 4,
    PAL_HEAD = 5,
    PAL_MALLET = 6,
    PAL_CRACK = 7,
    PAL_WOOD = 8,
    PAL_PAPER = 9,
    PAL_SKIN = 10
};

// The tape is SHELL, HEAD, MALLET. CRACK pays like HEAD and stays out.
struct Part {
    const char* name;
    int pay;
    bool decoy;
    int pal;
};

constexpr Part kPart[kParts] = {
    {"SHELL", 6, false, PAL_SHELL},
    {"HEAD", 9, false, PAL_HEAD},
    {"MALLET", 3, false, PAL_MALLET},
    {"CRACK", 9, true, PAL_CRACK},
};

constexpr int kTape[kTapeN] = {0, 1, 2};

struct Art {
    gs::Image drum;
    gs::Image skin;
    gs::Image stick;
    gs::Image shell;
    gs::Image head;
    gs::Image mallet;
    gs::Image crack;
    gs::Image drawer;
    gs::Image lamp;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace drumtape
