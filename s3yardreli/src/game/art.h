// S3 YARD RELIEF sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace yard {

enum Pal {
    PAL_HUD = 0,
    PAL_CRANE = 1,
    PAL_HAND = 2,
    PAL_SCRAP = 3,
    PAL_DOG = 4,
    PAL_WRECK = 5,
    PAL_BELL = 6,
    PAL_FX = 7,
    PAL_PROP = 8,
    PAL_FIELD = 12
};

struct Art {
    gs::Mipped cab, shack, beam, magnet, cable, bell;
    gs::Mipped hand[2];
    gs::Mipped dog[2];
    gs::Mipped scrap[2];
    gs::Mipped relief[2];
    gs::Mipped wreck;
    gs::Mipped pile[2];
    gs::Mipped lamp, drum, pad;
    gs::Mipped link, spark, puff, smoke, shadow, moon;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace yard
