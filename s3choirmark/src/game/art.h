// S3 CHOIRMARK pictures. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"

namespace choirmark {

enum Pal {
    PAL_INK = 0,
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
    gs::Mipped mark;
    gs::Mipped title;
    gs::Mipped done;
    gs::Image bar;
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace choirmark
