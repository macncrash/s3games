// S3 TRAM BOOM sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace tramboom {

enum Pal {
    PAL_HUD = 0,
    PAL_TRAM = 1,
    PAL_DRIVE = 2,
    PAL_BOOM = 3,
    PAL_CITY = 4,
    PAL_ALERT = 5,
    PAL_OK = 6,
    PAL_DUSK = 7
};

struct Art {
    gs::Mipped tram;
    gs::Mipped wheel;
    gs::Mipped drive;
    gs::Mipped boom;
    gs::Mipped shed;
    gs::Mipped block;
    gs::Mipped pole;
    gs::Mipped wire;
    gs::Mipped rail;
    gs::Mipped bumper;
    gs::Mipped glyph[96];
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace tramboom
