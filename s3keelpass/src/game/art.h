// S3 KEEL PASS sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace keelpass {

enum Pal : int {
    PAL_HUD = 0,
    PAL_HULL = 1,
    PAL_SAIL = 2,
    PAL_CLIFF = 3,
    PAL_PINE = 4,
    PAL_FOAM = 5,
    PAL_CLOUD = 6,
    PAL_CREW = 7,
    PAL_WIN = 8,
    PAL_ALERT = 9,
    PAL_GOLD = 10,
};

struct Art {
    gs::Mipped hull;
    gs::Mipped sail;
    gs::Mipped cliff;
    gs::Mipped pine;
    gs::Mipped foam;
    gs::Mipped cloud;
    gs::Mipped title;
    gs::Mipped clear;
    gs::Mipped rock;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace keelpass
