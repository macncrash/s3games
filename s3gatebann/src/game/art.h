// S3 GATE BANN sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace bann {

enum Pal {
    PAL_HUD = 0,
    PAL_STONE = 1,
    PAL_EARTH = 2,
    PAL_BANNER = 3,
    PAL_PLAYER = 4,
    PAL_SENTRY = 5,
    PAL_WOOD = 6,
    PAL_FX = 7,
    PAL_ALERT = 8
};

struct Art {
    gs::Mipped stand, runA, runB, swing;
    gs::Mipped sentry[3];
    gs::Mipped banner[2];
    gs::Mipped pole;
    gs::Mipped tower;
    gs::Mipped lintel;
    gs::Mipped bars;
    gs::Mipped door;
    gs::Mipped pennant;
    gs::Mipped slab;
    gs::Mipped stake;
    gs::Mipped tent;
    gs::Mipped brazier;
    gs::Mipped flame[2];
    gs::Mipped sun;
    gs::Mipped shadow;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace bann
