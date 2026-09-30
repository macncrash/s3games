// S3 HELIBOX sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace heli {

enum Pal {
    PAL_HUD = 0,
    PAL_SHIP = 1,
    PAL_BOX = 2,
    PAL_WORLD = 3,
    PAL_FX = 4
};

struct Art {
    gs::Mipped body;
    gs::Mipped rotor[3];
    gs::Mipped box;
    gs::Mipped ground;
    gs::Mipped hill;
    gs::Mipped cloud;
    gs::Mipped sun;
    gs::Mipped glyph[96];
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace heli
