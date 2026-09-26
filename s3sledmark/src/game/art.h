// S3 SLED MARK sprites. Drawn into VRAM at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace sledmark {

enum Pal : int {
    PAL_HUD = 0,
    PAL_TEAM = 1,
    PAL_RIVAL = 2,
    PAL_PINE = 3,
    PAL_WOOD = 4,
    PAL_MARK = 5,
    PAL_FX = 6,
    PAL_WIN = 7,
    PAL_ALERT = 8,
    PAL_BANNER = 9,
    PAL_MAP = 10,
    PAL_ROCK = 11,
    PAL_SNOW = 12,
    PAL_ICE = 13
};

struct Art {
    gs::Mipped sled[16];
    gs::Mipped disc, stake, pine, cabin, rock, crack, puff, pin, dot, panel;
    gs::Mipped title, setDown, onMark, crewTook, missed, buried, paused;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace sledmark
