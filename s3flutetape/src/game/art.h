// S3 FLUTETAPE pictures. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace flutetape {

constexpr int kNotes = 4;
constexpr int kTapeN = 3;
constexpr int kDieAt = 72;
constexpr int kSweetLo = 30;
constexpr int kSweetHi = 42;

enum Pal {
    PAL_HUD = 0,
    PAL_GOLD = 1,
    PAL_DIM = 2,
    PAL_BAD = 3,
    PAL_LIP = 4,
    PAL_BODY = 5,
    PAL_FOOT = 6,
    PAL_WHISTLE = 7,
    PAL_WOOD = 8,
    PAL_PAPER = 9,
    PAL_SILVER = 10
};

// The tape is LIP, BODY, FOOT. WHISTLE pays like LIP and stays out of the drawer.
struct Note {
    const char* name;
    int pay;
    bool decoy;
    int pal;
    float hz;
};

constexpr Note kNote[kNotes] = {
    {"LIP", 4, false, PAL_LIP, 523.f},
    {"BODY", 7, false, PAL_BODY, 659.f},
    {"FOOT", 5, false, PAL_FOOT, 784.f},
    {"WHISTLE", 4, true, PAL_WHISTLE, 1046.f},
};

constexpr int kTape[kTapeN] = {0, 1, 2};

struct Art {
    gs::Image flute;
    gs::Image hole;
    gs::Image lip;
    gs::Image body;
    gs::Image foot;
    gs::Image whistle;
    gs::Image drawer;
    gs::Image breath;
    gs::Image player;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace flutetape
