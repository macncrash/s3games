// S3 OUTPOST sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"

namespace outpost {

enum Pal {
    PAL_HUD = 0,
    PAL_SENT = 1,
    PAL_THEM = 2,
    PAL_BRUTE = 3,
    PAL_FLARE = 4,
    PAL_HUT = 5,
    PAL_PINE = 6,
    PAL_FX = 7,
    PAL_GROUND = 8
};

struct Art {
    gs::Mipped sentry[2];
    gs::Mipped stalker[2];
    gs::Mipped brute[2];
    gs::Mipped hut, mast, tree, post;
    gs::Mipped flare, glow, bolt, glint, aim, chev, sun, shadow;
    gs::Mipped glyph[96];
    int font[96] = {};
    int grass = 1, moss = 2, dirt = 3, yard = 4;
    int pipOn = 5, pipDim = 6, pipMark = 7;
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace outpost
