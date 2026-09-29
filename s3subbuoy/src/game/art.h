// S3 SUB BUOY sprites. Drawn into sprite ROM at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace subby {

enum Pal : int {
    PAL_HUD = 0,
    PAL_SUB = 1,
    PAL_NUN = 2,
    PAL_CAN = 3,
    PAL_DOCK = 4,
    PAL_BUB = 5,
    PAL_KELP = 6,
    PAL_WRECK = 7,
    PAL_WAVE = 8,
    PAL_FISH = 9,
    PAL_BANNER = 10,
    PAL_WIN = 11,
    PAL_ALERT = 12,
    PAL_SONAR = 13,
    PAL_MAP = 14,
    PAL_LAMP = 15
};

struct Art {
    gs::Mipped hull[8];
    gs::Mipped buoy[3];
    gs::Mipped quay, quayB, shed, pile, beam, crane;
    gs::Mipped wreck, kelp, fish[2], bubble, ring, lamp, pin, dot, panel;
    gs::Mipped title, line, same, paused;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace subby
