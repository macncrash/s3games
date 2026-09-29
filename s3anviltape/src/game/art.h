// S3 ANVILTAPE pictures. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace anviltape {

constexpr int kBars = 4;
constexpr int kTapeN = 3;
constexpr int kDieAt = 84;
constexpr int kSweetLo = 46;
constexpr int kSweetHi = 58;

enum Pal {
    PAL_HUD = 0,
    PAL_GOLD = 1,
    PAL_DIM = 2,
    PAL_BAD = 3,
    PAL_IRON = 4,
    PAL_WOOD = 5,
    PAL_BRASS = 6,
    PAL_COPPER = 7,
    PAL_SLAG = 8,
    PAL_PAPER = 9
};

// The tape is BILLET, BLOOM, TONGS. SLAG pays the same as BLOOM and stays out.
struct Bar {
    const char* name;
    int pay;
    bool decoy;
    int pal;
};

constexpr Bar kBar[kBars] = {
    {"BILLET", 5, false, PAL_IRON},
    {"BLOOM", 8, false, PAL_BRASS},
    {"TONGS", 3, false, PAL_COPPER},
    {"SLAG", 8, true, PAL_SLAG},
};

constexpr int kTape[kTapeN] = {0, 1, 2};

struct Art {
    gs::Image anvil;
    gs::Image hammer;
    gs::Image billet;
    gs::Image bloom;
    gs::Image tongs;
    gs::Image slag;
    gs::Image drawer;
    gs::Image lamp;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace anviltape
