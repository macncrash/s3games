// S3 RICKSHAW MARK sprites. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace rickmark {

enum Pal : int {
    PAL_HUD = 0,
    PAL_CAB = 1,
    PAL_STREET = 2,
    PAL_MARK = 3,
    PAL_BANNER = 4,
    PAL_ALERT = 5,
    PAL_WIN = 6,
    PAL_CREW = 7,
    PAL_LAMP = 8
};

struct Art {
    gs::Mipped cabUp[2];
    gs::Mipped cabDown;
    gs::Mipped shade;
    gs::Mipped stall, awning, lamp;
    gs::Mipped chalk, flag;
    gs::Mipped hatch, stripe;
    gs::Mipped clock, walker[2];
    gs::Mipped title, setdown, missed, offmark, rolling, crew, paused;
    gs::Mipped glyph[96];
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace rickmark
