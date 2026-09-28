// S3 BARGE LANE sprites. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace bargelane {

enum Pal {
    PAL_HUD = 0,
    PAL_HULL = 1,
    PAL_BUOY = 2,
    PAL_BANK = 3,
    PAL_WIN = 4,
    PAL_ALERT = 5,
    PAL_SIGN = 6,
    PAL_STACK = 7,
    PAL_CANAL = 12
};

struct Art {
    gs::Mipped hull, cabin, stack, buoy, reed, mill, gate, wake;
    gs::Mipped title, held, left, missed, stay, start;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace bargelane
