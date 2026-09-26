// S3 SOLITAIRE pictures. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace sol {

constexpr int CARD_W = 36;
constexpr int CARD_H = 50;
constexpr int NRANK = 7;
constexpr int NSUIT = 4;
constexpr int NCARD = NRANK * NSUIT;

enum Pal {
    PAL_HUD = 0,
    PAL_GOLD = 1,
    PAL_ALERT = 2,
    PAL_WIN = 3,
    PAL_FACE = 4,
    PAL_SLOT = 5,
    PAL_CURSOR = 6,
    PAL_FELT = 7,
    PAL_BRASS = 8,
    PAL_FILE = 9
};

struct Art {
    gs::Mipped face[NCARD];
    gs::Mipped slot[NSUIT];
    gs::Mipped cursor;
    gs::Mipped file;
    gs::Mipped shadow;
    gs::Mipped rule;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace sol
