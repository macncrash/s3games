// S3 TRAM PASS pictures. Drawn into VRAM at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace trampass {

enum Pal {
    PAL_HUD = 0,
    PAL_GOLD = 1,
    PAL_ALERT = 2,
    PAL_TRAM = 3,
    PAL_RIVAL = 4,
    PAL_PINE = 5,
    PAL_MAST = 6,
    PAL_BANNER = 7,
    PAL_FX = 8,
    PAL_RANGE = 10,
    PAL_ROAD = 12
};

struct Art {
    gs::Mipped tram;
    gs::Mipped rival;
    gs::Mipped pine, mast, rock;
    gs::Mipped passBan, yardBan;
    gs::Mipped chevL, chevR;
    gs::Mipped flake, shadow;
    gs::Image title, sub;
    gs::Image go, clear, closed, derail;
    int font[128] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace trampass
