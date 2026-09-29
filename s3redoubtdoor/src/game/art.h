// S3 REDOUBT DOOR sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace door {

enum Pal {
    PAL_HUD = 0,
    PAL_STONE = 1,
    PAL_WOOD = 2,
    PAL_YOU = 3,
    PAL_FOE = 4,
    PAL_IRON = 5,
    PAL_ALERT = 6,
    PAL_OK = 7,
    PAL_NIGHT = 8
};

struct Art {
    gs::Mipped wall, door, doorhurt, bar, you[2], foe[2], ram, spark;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace door
