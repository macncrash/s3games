// S3 HELI LANE sprites. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace helilane {

enum Pal {
    PAL_HUD = 0,
    PAL_SHIP = 1,
    PAL_RIVAL = 2,
    PAL_PYLON = 3,
    PAL_WIN = 4,
    PAL_ALERT = 5,
    PAL_SIGN = 6,
    PAL_ROTOR = 7,
    PAL_LANE = 12
};

struct Art {
    gs::Mipped body, disc, fin, shadow, pylon, gate, cloud;
    gs::Mipped title, held, left, missed, stay, start;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace helilane
