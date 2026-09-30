// S3 KARTBOOM sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace kartboom {

enum Pal {
    PAL_HUD = 1,
    PAL_KART = 2,
    PAL_DRIVE = 3,
    PAL_CONE = 4,
    PAL_BOOM = 5,
    PAL_ROAD = 12
};

struct Art {
    gs::Mipped kart;
    gs::Mipped kartEmpty;
    gs::Mipped drive;
    gs::Mipped cone;
    gs::Mipped boom;
    gs::Image glyph[96];
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace kartboom
