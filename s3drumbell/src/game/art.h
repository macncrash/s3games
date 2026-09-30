// S3 DRUMBELL sprites. Drawn into VDP RAM at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace drumbell {

constexpr int kTries = 3;
constexpr int kMarks = 4;

enum Pal {
    PAL_HUD = 0,
    PAL_GOLD = 1,
    PAL_RED = 2,
    PAL_GREEN = 3,
    PAL_DIM = 4,
    PAL_DRUM = 5,
    PAL_PLAYER = 6,
    PAL_BELL = 7,
    PAL_WOOD = 8,
    PAL_FX = 9
};

struct Art {
    gs::Mipped drum;
    gs::Mipped head;
    gs::Mipped headOn;
    gs::Mipped stickUp;
    gs::Mipped stickDown;
    gs::Mipped player;
    gs::Mipped bell;
    gs::Mipped clapper;
    gs::Mipped lamp;
    gs::Mipped curtain;
    gs::Mipped beater;
    gs::Mipped stool;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace drumbell
