// S3 METRO MARK sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace metromark {

enum Pal : int {
    PAL_HUD = 0,
    PAL_METRO = 1,
    PAL_PLAT = 2,
    PAL_MARK = 3,
    PAL_LAMP = 4,
    PAL_ARCH = 5,
    PAL_ALERT = 6,
    PAL_WIN = 7,
    PAL_ROAD = 12
};

struct Art {
    gs::Mipped metro, shade, pant;
    gs::Mipped arch, lamp, bench;
    gs::Mipped plat, stripe, mark;
    gs::Mipped title, crew, onmark, missed, start;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace metromark
