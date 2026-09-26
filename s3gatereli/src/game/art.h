// S3 GATE RELIEF sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace gate {

enum Pal {
    PAL_HUD = 0,
    PAL_STONE = 1,
    PAL_SENTRY = 2,
    PAL_RAIDER = 3,
    PAL_RAM = 4,
    PAL_FX = 5,
    PAL_BELL = 6,
    PAL_NIGHT = 7,
    PAL_FIELD = 12
};

struct Art {
    gs::Mipped tower;
    gs::Mipped sentry[2];
    gs::Mipped runner[2];
    gs::Mipped shield;
    gs::Mipped ram;
    gs::Mipped relief[2];
    gs::Mipped spear;
    gs::Mipped bolt;
    gs::Mipped flame[2];
    gs::Mipped post;
    gs::Mipped bell;
    gs::Mipped rope;
    gs::Mipped moon;
    gs::Mipped star;
    gs::Mipped puff;
    gs::Mipped banner;
    gs::Mipped grate;
    gs::Mipped shadow;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace gate
