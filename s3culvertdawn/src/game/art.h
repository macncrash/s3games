// S3 CULVERTDAWN sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace culvertdawn {

enum Pal {
    PAL_HUD = 0,
    PAL_STONE = 1,
    PAL_WATCH = 2,
    PAL_FLARE = 3,
    PAL_NIGHT = 4,
    PAL_ASH = 5
};

struct Art {
    gs::Image culvert;
    gs::Image moon;
    gs::Image star;
    gs::Image pot;
    gs::Image flame;
    gs::Image ash;
    gs::Image watch[2];
    gs::Image title;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace culvertdawn
