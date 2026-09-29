// S3 CHOIR SEVEN pictures. Drawn into VRAM at boot. No asset files.
#pragma once
#include "console/gfx.h"

namespace choirseven {

enum Pal {
    PAL_HUD = 0,
    PAL_NAVE = 1,
    PAL_TREBLE = 2,
    PAL_ALTO = 3,
    PAL_BASS = 4,
    PAL_GOLD = 5,
    PAL_THEM = 6,
    PAL_ALERT = 7
};

struct Art {
    int font[96] = {};
    gs::Mipped singer[3];
    gs::Mipped note;
    gs::Mipped bar;
    gs::Mipped pip;
    gs::Mipped wordChoir;
    gs::Mipped wordSeven;
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace choirseven
