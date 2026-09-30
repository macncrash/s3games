// S3 WHARF MAGA pictures. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace wharfmaga {

enum Pal {
    PAL_HUD = 0,
    PAL_KEEP = 1,
    PAL_BOARD = 2,
    PAL_SWING = 3,
    PAL_DINGHY = 4,
    PAL_BRASS = 5,
    PAL_FX = 6,
    PAL_OK = 7,
    PAL_ALERT = 8,
    PAL_PILE = 9,
    PAL_GULL = 10,
    PAL_CRATE = 11
};

struct Art {
    gs::Mipped keeper[2];
    gs::Mipped boarder[2];
    gs::Mipped swinger[2];
    gs::Mipped dinghy;
    gs::Mipped pile;
    gs::Mipped crate;
    gs::Mipped gull[2];
    gs::Mipped brass;
    gs::Mipped flash;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace wharfmaga
