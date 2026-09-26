// S3 SLED PASS pictures. Drawn into VRAM at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace sledpass {

enum Pal {
    PAL_HUD = 0,
    PAL_GOLD = 1,
    PAL_ALERT = 2,
    PAL_TEAM = 3,
    PAL_PINE = 4,
    PAL_TIMBER = 5,
    PAL_ROCK = 6,
    PAL_FX = 7,
    PAL_BANNER = 8,
    PAL_RANGE = 10,
    PAL_SNOW = 12
};

struct Art {
    gs::Mipped team[2];  // 0 straight, 1 leaned left (flip for right)
    gs::Mipped pine, stake, cabin, boulder, drift;
    gs::Mipped post, mushBan, passBan;
    gs::Mipped flake, shadow;
    gs::Image title, sub;
    gs::Image go, clear, storm, buried;
    int font[128] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace sledpass
