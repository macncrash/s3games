// S3 HEADER TURN pictures. Drawn into sprite ROM at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace headerturn {

enum Pal : int {
    PAL_HUD = 0,
    PAL_ALERT = 1,
    PAL_WIN = 2,
    PAL_BANNER = 3,
    PAL_TAG = 4,
    PAL_BOAT = 5,
    PAL_BUOY = 6,
    PAL_REEF = 7,
    PAL_SHED = 8,
    PAL_FINISH = 9,
    PAL_FOAM = 10,
    PAL_GULL = 11,
    PAL_SEA = 12,
    PAL_SKY = 13,
    PAL_MARK = 14,
};

constexpr int kHeels = 5;

struct Art {
    gs::Mipped boat[kHeels];
    gs::Mipped wreck;
    gs::Mipped stake;
    gs::Mipped reef;
    gs::Mipped shed;
    gs::Mipped finish;
    gs::Mipped mark[3];
    gs::Mipped wake;
    gs::Mipped gull[2];
    gs::Mipped sun;
    gs::Mipped cloud;
    gs::Mipped shade;
    gs::Mipped title;
    gs::Mipped tipped;
    gs::Mipped missed;
    gs::Mipped made;
    gs::Mipped held;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace headerturn
