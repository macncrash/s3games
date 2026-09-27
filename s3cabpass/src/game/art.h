// S3 CAB PASS pictures. Drawn into VRAM at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace cabpass {

enum Pal {
    PAL_HUD = 0,
    PAL_GOLD = 1,
    PAL_ALERT = 2,
    PAL_CAB = 3,
    PAL_RIVAL = 4,
    PAL_PINE = 5,
    PAL_ROCK = 6,
    PAL_BANNER = 7,
    PAL_FX = 8,
    PAL_RANGE = 10,
    PAL_ROAD = 12
};

struct Art {
    gs::Mipped cab[2];  // 0 straight, 1 leaned left (flip for right)
    gs::Mipped rival;
    gs::Mipped pine, rock, lamp, post;
    gs::Mipped passBan, fareBan;
    gs::Mipped flake, shadow;
    gs::Image title, sub;
    gs::Image go, clear, closed, ditch;
    int font[128] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace cabpass
