// S3 FERRY KILO sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace fkilo {

enum Pal {
    PAL_HUD = 0,
    PAL_AMBER = 1,
    PAL_BAD = 2,
    PAL_GOOD = 3,
    PAL_SHIP = 4,
    PAL_PADDLE = 5,
    PAL_LORRY = 6,
    PAL_GANTRY = 7,
    PAL_WATER = 8,
    PAL_BANK = 9,
    PAL_SKY = 10,
    PAL_PIER = 11,
    PAL_BANNER = 12,
    PAL_POST = 13,
    PAL_SHADE = 14
};

struct Hull {
    gs::Mipped img;
    float ax = 0, ay = 0;
    float ppm = 10;
};

struct Art {
    Hull hull[5];
    gs::Mipped wheel[4];
    float wheelRim = 0.9f;
    gs::Mipped mill, bed, beam, cable;
    float millAx = 0, millAy = 0, millSpan = 1;
    gs::Mipped water, bank, cloud, rail, sun;
    gs::Mipped reed, gull[2], flag[2];
    gs::Mipped pole, banner, shade;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace fkilo
