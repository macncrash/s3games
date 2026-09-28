// S3 LANTERNTAPE pictures. Drawn at boot. No asset files.
#pragma once

#include "console/gfx.h"
#include "console/vdp.h"

namespace lanterntape {

enum Pal {
    PAL_HUD = 0,
    PAL_LAMP = 1,
    PAL_WALK = 2,
    PAL_POST = 3,
    PAL_SLIP = 4,
    PAL_MOON = 5,
    PAL_LIT = 6,
    PAL_INK = 7
};

constexpr int kTapeN = 3;

struct Post {
    const char* name;
    int pay;
    float x;
    bool decoy;
};

// The tape is the first three. SNUFF pays the same as WICK and stays out.
constexpr Post kPost[4] = {
    {"WICK", 5, 250.f, false},
    {"GLASS", 3, 168.f, false},
    {"HOOK", 4, 46.f, false},
    {"SNUFF", 5, 108.f, true},
};

struct Art {
    gs::Mipped walker;
    gs::Mipped lamp;
    gs::Mipped flame;
    gs::Mipped post;
    gs::Mipped slip;
    gs::Mipped moon;
    gs::Mipped star;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace lanterntape
