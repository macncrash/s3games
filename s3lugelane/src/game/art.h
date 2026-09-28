// S3 LUGE LANE pictures. Drawn into VRAM at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace lugelane {

enum Pal : int {
    PAL_HUD = 0,
    PAL_RIDER = 1,
    PAL_WALL = 2,
    PAL_ROCK = 3,
    PAL_ICE = 4,
    PAL_GATE = 5,
    PAL_SPRAY = 6,
    PAL_SKY = 7,
    PAL_ALERT = 8,
    PAL_WIN = 9,
    PAL_BANNER = 10,
    PAL_TAG = 11,
    PAL_LANE = 12,
    PAL_END = 13,
    PAL_SPARE = 15
};

struct Art {
    gs::Mipped rider[3];
    gs::Mipped pod;
    gs::Mipped stake, rock, lamp;
    gs::Mipped post, bar, flag;
    gs::Mipped spray, flake, shadow;
    gs::Mipped sun, cloud;
    gs::Mipped title, held, whole, left, missed, paused, end;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace lugelane
