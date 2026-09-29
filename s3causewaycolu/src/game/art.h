// S3 CAUSEWAY COLUMN sprites. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace cway {

enum Pal {
    PAL_HUD = 0,
    PAL_TRUCK = 1,
    PAL_CHAIN = 2,
    PAL_KEEPER = 3,
    PAL_LAMP = 4,
    PAL_STONE = 5,
    PAL_OK = 6,
    PAL_ALERT = 7,
    PAL_ROAD = 12
};

struct Art {
    gs::Mipped truck;
    gs::Mipped chain;
    gs::Mipped post;
    gs::Mipped keeper;
    gs::Mipped lamp;
    gs::Mipped glyph[96];
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace cway
