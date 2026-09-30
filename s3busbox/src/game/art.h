// S3 BUS BOX pictures. Drawn into VRAM at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace busbox {

// Bus spans [x - kRear, x + kFront]. The whole body has to sit in the box.
constexpr float kRear = 5.55f;
constexpr float kFront = 6.05f;
constexpr float kBoxL = 68.0f;
constexpr float kBoxR = 82.4f;
constexpr float kGoal = 74.15f;
constexpr float kEnd = 86.2f;  // nose past this misses the end of the leg
constexpr float kWest = 6.0f;
constexpr float kPpm = 10.6f;
constexpr float kRoadY = 176.f;

enum Pal : int {
    PAL_HUD = 0,
    PAL_BUS = 1,
    PAL_GLASS = 2,
    PAL_ROAD = 3,
    PAL_BOX = 4,
    PAL_STOP = 5,
    PAL_TOWN = 6,
    PAL_END = 7,
    PAL_FOLK = 8,
    PAL_DUST = 9,
    PAL_WIN = 10,
    PAL_ALERT = 11,
    PAL_BANNER = 12,
    PAL_NIGHT = 13,
};

struct Art {
    gs::Mipped bus;
    gs::Mipped wheel[2];
    gs::Mipped dust;
    gs::Mipped road;
    gs::Mipped stripe;
    gs::Mipped kerb;
    gs::Mipped post;
    gs::Mipped hatch;
    gs::Mipped shelter;
    gs::Mipped barrier;
    gs::Mipped block;
    gs::Mipped lamp;
    gs::Mipped rider;
    gs::Mipped title;
    gs::Mipped boxWord;
    gs::Mipped stopped;
    gs::Mipped missed;
    gs::Mipped outside;
    gs::Mipped paused;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace busbox
