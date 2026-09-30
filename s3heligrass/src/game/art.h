// S3 HELIGRASS sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace heli {

enum Pal {
    PAL_HUD = 0,
    PAL_SHIP = 1,
    PAL_RIVAL = 2,
    PAL_WORLD = 3,
    PAL_GRASS = 4
};

struct Art {
    gs::Mipped body;
    gs::Mipped rotor[3];
    gs::Mipped rival;
    gs::Mipped grass;
    gs::Mipped water;
    gs::Mipped disc;
    gs::Mipped tree;
    gs::Mipped sock;
    gs::Mipped cloud;
    gs::Mipped glyph[96];
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace heli
