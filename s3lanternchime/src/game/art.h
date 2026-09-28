// S3 LANTERNCHIME sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace lanternchime {

enum Pal {
    PAL_HUD = 0,
    PAL_SKY = 1,
    PAL_YARD = 2,
    PAL_FIRE = 3,
    PAL_BODY = 4,
    PAL_IRON = 5,
    PAL_CLOCK = 6,
    PAL_GLOW = 7
};

constexpr int kHands = 16;

struct Art {
    gs::Mipped bearer;
    gs::Mipped pole;
    gs::Mipped cage;
    gs::Mipped flame[2];
    gs::Mipped glow;
    gs::Mipped bell;
    gs::Mipped rope;
    gs::Mipped moon;
    gs::Mipped tower;
    gs::Mipped face;
    gs::Mipped hand[kHands];
    gs::Mipped hourHand;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace lanternchime
