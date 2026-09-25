// Pictures drawn at boot. Nothing is loaded from a file.
#pragma once

#include "console/gfx.h"
#include "console/vdp.h"

namespace fort {

enum Pal {
    PAL_HUD = 0,
    PAL_GOLD = 1,
    PAL_ALERT = 2,
    PAL_DIM = 3,
    PAL_WARDEN = 4,
    PAL_RAIDER = 5,
    PAL_SHIELD = 6,
    PAL_RAM = 7,
    PAL_GATE = 8,
    PAL_FIRE = 9,
    PAL_BANNER = 10,
    PAL_MOON = 11,
    PAL_ROAD = 12,
    PAL_BOLT = 13,
    PAL_DUST = 14
};

struct Art {
    gs::Mipped glyph[96];
    int font[96] = {};
    int pipOk = 1, pipMid = 1, pipLow = 1, pipOff = 1;
    gs::Mipped warden[3];
    gs::Mipped raider[2];
    gs::Mipped shield[2];
    gs::Mipped ram[2];
    gs::Mipped bolt;
    gs::Mipped tower;
    gs::Mipped door, doorBroke;
    gs::Mipped wall;
    gs::Mipped banner;
    gs::Mipped torch[2];
    gs::Mipped moon;
    gs::Mipped star;
    gs::Mipped puff, spark;
    gs::Mipped shadow;
    gs::Mipped brace;
    gs::Mipped clock[4];
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace fort
