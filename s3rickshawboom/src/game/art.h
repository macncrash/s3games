// S3 RICKSHAW BOOM sprites. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace rickboom {

enum Pal : int {
    PAL_HUD = 0,
    PAL_CAB = 1,
    PAL_STREET = 2,
    PAL_BOOM = 3,
    PAL_DRIVE = 4,
    PAL_BANNER = 5,
    PAL_ALERT = 6,
    PAL_WIN = 7,
    PAL_CREW = 8,
    PAL_LAMP = 9
};

struct Art {
    gs::Mipped cab[2];
    gs::Mipped shade;
    gs::Mipped drive;
    gs::Mipped boom, post, hook;
    gs::Mipped stall, awning, lamp, crate;
    gs::Mipped hatch, stripe;
    gs::Mipped clock;
    gs::Mipped title, delivered, missed, offBoom, shortB, broke, crew, paused;
    gs::Mipped glyph[96];
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace rickboom
