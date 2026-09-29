// S3 VIADUCT DOOR pictures. Drawn into VRAM at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace viaduct {

enum Pal {
    PAL_TEXT = 0,
    PAL_DUSK = 1,
    PAL_ALERT = 2,
    PAL_GOOD = 3,
    PAL_STONE = 4,
    PAL_COAT = 5,
    PAL_IRON = 6,
    PAL_LAMP = 7,
    PAL_FLAG = 8,
    PAL_GORGE = 9,
    PAL_SKY = 10,
    PAL_SPARK = 11,
    PAL_DECK = 12
};

struct Art {
    gs::Mipped slab;
    gs::Mipped pier;
    gs::Mipped crown;
    gs::Mipped warden[2];
    gs::Mipped gust[2];
    gs::Mipped hitch;
    gs::Mipped chock;
    gs::Mipped flag;
    gs::Mipped lamp;
    gs::Mipped rivet;
    gs::Mipped mote;
    gs::Mipped glyph[96];
    int font[96] = {};
    int course = 1;
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace viaduct
