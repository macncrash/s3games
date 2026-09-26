// S3 GATE DOOR sprites. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace gatedoor {

enum Pal {
    PAL_HUD = 0,
    PAL_GOLD = 1,
    PAL_ALERT = 2,
    PAL_GOOD = 3,
    PAL_STONE = 4,
    PAL_WOOD = 5,
    PAL_IRON = 6,
    PAL_COAT = 7,
    PAL_RAIDER = 8,
    PAL_FIRE = 9,
    PAL_MOON = 10,
    PAL_GRASS = 11,
    PAL_ROAD = 12
};

struct Art {
    gs::Mipped pier;
    gs::Mipped beam;
    gs::Mipped leaf;
    gs::Mipped latch;
    gs::Mipped hinge;
    gs::Mipped catchPlate;
    gs::Mipped wedge;
    gs::Mipped keeper[2];
    gs::Mipped raider[2];
    gs::Mipped hand;
    gs::Mipped ram;
    gs::Mipped lantern[2];
    gs::Mipped bell;
    gs::Mipped moon;
    gs::Mipped star;
    gs::Mipped tuft;
    gs::Mipped tree;
    gs::Mipped spark;
    gs::Mipped chip;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace gatedoor
