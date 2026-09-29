// S3 TRENCH WELL pictures. Drawn into VRAM at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace trench {

enum Pal {
    PAL_HUD = 0,
    PAL_FIELD = 1,
    PAL_YOU = 2,
    PAL_FOE = 3,
    PAL_SAP = 4,
    PAL_SHELL = 5,
    PAL_WELL = 6,
    PAL_FX = 7,
    PAL_AMBER = 8,
    PAL_ALERT = 9,
    PAL_OK = 10
};

constexpr float WELL_X = 160.f;
constexpr float WELL_Y = 158.f;
constexpr float PARA = 176.f;

struct Art {
    gs::Mipped rifle[2];
    gs::Mipped raider[2];
    gs::Mipped sapper[2];
    gs::Mipped shell[2];
    gs::Mipped well;
    gs::Mipped crack;
    gs::Mipped shot;
    gs::Mipped flash;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace trench
