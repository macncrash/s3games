// Pictures drawn at boot. No asset files.
#pragma once

#include "console/gfx.h"
#include "console/vdp.h"

namespace heli {

enum Pal {
    PAL_HUD = 0,
    PAL_GROUND = 1,
    PAL_HELI = 2,
    PAL_MARK = 3,
    PAL_CLOUD = 4,
    PAL_SUN = 5,
    PAL_TREE = 6
};

struct Art {
    gs::Mipped heli;
    gs::Mipped rotor;
    gs::Mipped mark;
    gs::Mipped ground;
    gs::Mipped cloud;
    gs::Mipped sun;
    gs::Mipped tree;
    gs::Mipped sock;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace heli
