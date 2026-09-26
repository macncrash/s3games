// S3 SLED BOX sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"

namespace sledbox {

enum Pal : int {
    PAL_HUD = 0,
    PAL_SLED = 1,
    PAL_CREW = 2,
    PAL_STAKE = 3,
    PAL_WOOD = 4,
    PAL_SPRAY = 5,
    PAL_PINE = 6,
    PAL_BANNER = 7,
    PAL_WIN = 8,
    PAL_ALERT = 9,
    PAL_CLOCK = 10,
    PAL_BOX = 11,
    PAL_BIRD = 12,
    PAL_SNOW = 13,
    PAL_LAMP = 14
};

struct Art {
    gs::Mipped sled[16];
    gs::Mipped clock[12];
    gs::Mipped stake, pine, cabin, lamp, drift, spray, pin, dot, box;
    gs::Mipped bird[2];
    gs::Mipped title, stopIn, crewIs;
    gs::Mipped stopped, ahead, paused;
    gs::Mipped crewTook, shortOf, outside, slid;
    gs::Mipped boxTag, crewTag;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace sledbox
