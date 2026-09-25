// S3 CHOIR pictures. Drawn into VRAM at boot. No asset files.
#pragma once
#include "console/gfx.h"

namespace choir {

enum Pal {
    PAL_HUD = 0,
    PAL_NAVE = 1,
    PAL_TREBLE = 2,
    PAL_ALTO = 3,
    PAL_BASS = 4,
    PAL_GOLD = 5,
    PAL_FX = 6,
    PAL_ALERT = 7
};

struct Art {
    int font[96] = {};
    gs::Mipped singer[3][2];
    gs::Mipped note;
    gs::Mipped diamond;
    gs::Mipped fermata;
    gs::Mipped halo;
    gs::Mipped mote;
    gs::Mipped title;
    gs::Mipped together;
    gs::Mipped stopped;
    gs::Mipped anthem;
    gs::Mipped letter[3];
    gs::Image rule;
    gs::Image staff;
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace choir
