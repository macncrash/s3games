// S3 DEPOT POUC sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace pouc {

enum Pal {
    PAL_HUD = 0,
    PAL_WALL = 1,
    PAL_FLOOR = 2,
    PAL_POUCH = 3,
    PAL_PLAYER = 4,
    PAL_STEEL = 5,
    PAL_WOOD = 6,
    PAL_FX = 7,
    PAL_ALERT = 8,
    PAL_PIT = 9,
    PAL_DOOR = 10,
    PAL_GO = 11
};

struct Art {
    gs::Mipped stand, runA, runB, duck, leap;
    gs::Mipped pouch[2];
    gs::Mipped desk, crate, barrel, bay, mat;
    gs::Mipped lamp, flame[2];
    gs::Mipped hole, lip, deck;
    gs::Mipped door, hook, cable, beam, post;
    gs::Mipped signal, shadow;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace pouc
