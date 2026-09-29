// Pictures drawn at boot. Nothing is loaded from a file.
#pragma once

#include "console/gfx.h"
#include "console/vdp.h"

namespace choirchime {

enum Pal {
    PAL_HUD = 0,
    PAL_NAVE = 1,
    PAL_TREBLE = 2,
    PAL_ALTO = 3,
    PAL_BASS = 4,
    PAL_GOLD = 5,
    PAL_FX = 6,
    PAL_ALERT = 7,
    PAL_CLOCK = 8
};

struct Art {
    int font[96] = {};
    gs::Mipped singer[3][2];
    gs::Mipped note;
    gs::Mipped diamond;
    gs::Mipped fermata;
    gs::Mipped halo;
    gs::Mipped bell;
    gs::Mipped title;
    gs::Image staff;
    gs::Image rule;
    gs::Image face;
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace choirchime
