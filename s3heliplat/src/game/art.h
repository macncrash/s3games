// S3 HELIPLAT sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace heliplat {

enum Pal {
    PAL_HUD = 0,
    PAL_HELI = 1,
    PAL_DECK = 2,
    PAL_POST = 3,
    PAL_HILL = 4,
    PAL_CLOUD = 5,
    PAL_MARK = 6,
    PAL_END = 7
};

struct Art {
    gs::Mipped body;
    gs::Mipped rotor[3];
    gs::Mipped deck;
    gs::Mipped post;
    gs::Mipped hill;
    gs::Mipped cloud;
    gs::Mipped mark;
    gs::Mipped glyph[96];
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace heliplat
