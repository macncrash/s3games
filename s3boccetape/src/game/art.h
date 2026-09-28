// S3 BOCCETAPE pictures. Drawn at boot. No asset files.
#pragma once

#include "console/gfx.h"
#include "console/vdp.h"

namespace boccetape {

enum Pal {
    PAL_HUD = 0,
    PAL_PAPER = 1,
    PAL_RED = 2,
    PAL_PALE = 3,
    PAL_GREEN = 4,
    PAL_WOOD = 5,
    PAL_DIRT = 6,
    PAL_TREE = 7,
    PAL_AIM = 8,
    PAL_GOLD = 9,
    PAL_PLAYER = 10,
    PAL_SLOT = 11,
    PAL_SHADE = 12,
    PAL_SKY = 13
};

constexpr int kTapeN = 3;
constexpr int kMaxBowls = 6;
constexpr float kHandX = 108.f;
constexpr float kHandY = 196.f;
constexpr float kDist0 = 46.f;
constexpr float kDist1 = 168.f;

struct Ring {
    const char* name;
    int pay;
    float x, y, r;
    bool decoy;
};

// The tape is the first three. CLUSTER pays the same as PALLINO and stays out.
constexpr Ring kRing[4] = {
    {"PALLINO", 7, 108.f, 64.f, 15.f, false},
    {"BOCCE", 4, 74.f, 98.f, 16.f, false},
    {"POINT", 3, 142.f, 118.f, 16.f, false},
    {"CLUSTER", 7, 150.f, 70.f, 14.f, true},
};

struct Art {
    gs::Mipped bowl;
    gs::Mipped pallino;
    gs::Mipped shadow;
    gs::Mipped ring;
    gs::Mipped slip;
    gs::Mipped slot;
    gs::Mipped tree;
    gs::Mipped pine;
    gs::Mipped player;
    int font[96] = {};
    int grass = 1;
    int dirt = 1;
    int rail = 1;
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace boccetape
