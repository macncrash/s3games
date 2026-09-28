// S3 SCULL PASS pictures. Drawn into sprite ROM at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace scullpass {

constexpr float kHalfL = 3.55f;
constexpr float kHalfB = 0.42f;

enum Pal : int {
    PAL_HUD = 0,
    PAL_HULL = 1,
    PAL_OAR = 2,
    PAL_CLIFF = 3,
    PAL_PINE = 4,
    PAL_TAPE = 5,
    PAL_STORM = 6,
    PAL_WIN = 7,
    PAL_ALERT = 8,
    PAL_BANNER = 9,
    PAL_DIM = 10,
    PAL_FOAM = 11,
};

struct Art {
    gs::Mipped hull[8];
    gs::Mipped oar;
    gs::Mipped cliff;
    gs::Mipped pine;
    gs::Mipped post;
    gs::Mipped tape;
    gs::Mipped title;
    gs::Mipped clearWord;
    gs::Mipped missed;
    gs::Mipped paused;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace scullpass
