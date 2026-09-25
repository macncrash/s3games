// Pictures drawn at boot. Nothing is loaded from a file.
#pragma once

#include "console/gfx.h"
#include "console/vdp.h"

namespace fair {

enum Pal {
    PAL_HUD = 0,
    PAL_GOLD = 1,
    PAL_RED = 2,
    PAL_BLUE = 3,
    PAL_PLAYER = 4,
    PAL_WOOD = 5,
    PAL_BRASS = 6,
    PAL_GROUND = 7,
    PAL_CREAM = 8,
    PAL_NIGHT = 9,
    PAL_PINK = 10
};

struct Art {
    gs::Mipped glyph[96];
    int font[96] = {};
    int tileEdge = 0, tilePlank = 0, tilePlank2 = 0, tileGrass = 0, tileGrass2 = 0;
    gs::Mipped kid[2];
    gs::Mipped ring, bottle, balloon, dart, bell, puck;
    gs::Mipped hammer[2];
    gs::Mipped tower, awning, gate, car, hub, stand;
    gs::Mipped bulb, moon, fluff, burst, bunt, board, plank, shadow;
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace fair
