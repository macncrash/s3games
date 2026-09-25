// S3 BATTERY sprites. Everything is drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace battery {

enum Pal {
    PAL_TEXT = 0,
    PAL_GOLD = 1,
    PAL_ALERT = 2,
    PAL_GOOD = 3,
    PAL_CAR = 4,
    PAL_TRUCK = 5,
    PAL_ARMOR = 6,
    PAL_GUN = 7,
    PAL_FX = 8,
    PAL_TREE = 9,
    PAL_POST = 10,
    PAL_HILL = 11,
    PAL_ROAD = 12
};

struct Art {
    gs::Mipped car, truck, tanker, armor, wreck;
    gs::Mipped gun, crew, bags;
    gs::Mipped tree, pine;
    gs::Mipped post, tape;
    gs::Mipped boom, smoke, shell, shadow, sight, cloud, sun;
    gs::Mipped glyph[96];
    int font[96] = {};
    int hill = 1;
    int hillHi = 1;
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace battery
