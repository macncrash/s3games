// S3 HELISLIP sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace slip {

enum Pal {
    PAL_HUD = 0,
    PAL_SHIP = 1,
    PAL_RIVAL = 2,
    PAL_SEA = 3,
    PAL_PIER = 4
};

struct Art {
    gs::Mipped body;
    gs::Mipped rotor[3];
    gs::Mipped rival;
    gs::Mipped pile;
    gs::Mipped deck;
    gs::Mipped wave;
    gs::Mipped buoy;
    gs::Mipped gull;
    gs::Mipped cloud;
    gs::Mipped gauge;
    gs::Mipped glyph[96];
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace slip
