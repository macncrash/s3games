// S3 OVENCHIME pictures. Drawn at boot. No asset files.
#pragma once

#include "console/gfx.h"
#include "console/vdp.h"

namespace ovenchime {

enum Pal {
    PAL_INK = 0,
    PAL_GOLD = 1,
    PAL_ALERT = 2,
    PAL_OK = 3,
    PAL_OVEN = 4,
    PAL_LOAF = 5,
    PAL_CLOCK = 6,
    PAL_DOOR = 7,
    PAL_FIRE = 8,
    PAL_HAND = 9
};

struct Art {
    gs::Image oven;
    gs::Image loaf;
    gs::Image face;
    gs::Image hand;
    gs::Image door;
    gs::Image flame;
    gs::Image pip;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace ovenchime
