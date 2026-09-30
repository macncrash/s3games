// S3 CULVERT RELIEF sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace culvert {

enum Pal {
    PAL_HUD = 0,
    PAL_STONE = 1,
    PAL_RAID = 2,
    PAL_SENTRY = 3,
    PAL_BELL = 4,
    PAL_BOLT = 5,
    PAL_LAMP = 6,
    PAL_WATER = 7,
    PAL_ROAD = 12
};

struct Art {
    gs::Mipped sentry;
    gs::Mipped sentryDuck;
    gs::Mipped raider;
    gs::Mipped runner;
    gs::Mipped grenadier;
    gs::Mipped bell;
    gs::Mipped bolt;
    gs::Mipped shot;
    gs::Mipped grenade;
    gs::Mipped archL;
    gs::Mipped archR;
    gs::Mipped lintel;
    gs::Mipped lip;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace culvert
