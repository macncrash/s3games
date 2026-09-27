// S3 HARBOR LADD pictures. Drawn into sprite ROM at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace harborladd {

enum Pal {
    PAL_HUD = 0,
    PAL_HERO = 1,
    PAL_WOOD = 2,
    PAL_PILE = 3,
    PAL_IRON = 4,
    PAL_WATER = 5,
    PAL_BUOY = 6,
    PAL_LAMP = 7
};

struct Art {
    gs::Mipped hero[2];
    gs::Mipped climb;
    gs::Mipped plank;
    gs::Mipped pile;
    gs::Mipped rung;
    gs::Mipped buoy;
    gs::Mipped crate;
    gs::Mipped gull;
    gs::Mipped lamp;
    gs::Image wordHarbor;
    gs::Image wordFar;
    gs::Image wordGo;
    gs::Image wordHeld;
    gs::Image wordOver;
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace harborladd
