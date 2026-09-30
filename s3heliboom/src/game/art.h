// S3 HELI BOOM sprites. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace heliboom {

enum Pal {
    PAL_HUD = 0,
    PAL_SHIP = 1,
    PAL_DRIVE = 2,
    PAL_BOOM = 3,
    PAL_WORLD = 4
};

struct Art {
    gs::Mipped body;
    gs::Mipped rotor[3];
    gs::Mipped drive;
    gs::Mipped sling;
    gs::Mipped girder;
    gs::Mipped post;
    gs::Mipped pad;
    gs::Mipped water;
    gs::Mipped cloud;
    gs::Mipped glyph[96];
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace heliboom
