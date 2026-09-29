// S3 RICKSHAW BOX sprites. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace rickbox {

enum Pal : int {
    PAL_HUD = 0,
    PAL_CAB = 1,
    PAL_STREET = 2,
    PAL_BOX = 3,
    PAL_BANNER = 4,
    PAL_ALERT = 5,
    PAL_WIN = 6,
    PAL_CREW = 7,
    PAL_LAMP = 8
};

struct Art {
    gs::Mipped cab[2];
    gs::Mipped shade;
    gs::Mipped stall, awning, lamp, crate;
    gs::Mipped hatch, stripe, post;
    gs::Mipped clock;
    gs::Mipped title, stopped, missed, outside, shortB, crew, paused;
    gs::Mipped glyph[96];
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace rickbox
