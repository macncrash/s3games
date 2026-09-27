// Pictures drawn at boot. Nothing is loaded from a file.
#pragma once

#include "console/gfx.h"
#include "console/vdp.h"

namespace golfchime {

constexpr float kTeeX = 72.f;
constexpr float kTeeY = 154.f;
constexpr float kCupX = 252.f;
constexpr float kCupY = 96.f;
constexpr float kCupR = 9.f;
constexpr float kBallR = 4.f;
constexpr int kBalls = 3;
constexpr int kGraceSec = 24;
constexpr int kFpc = 6;

enum Pal {
    PAL_GREEN = 0,
    PAL_BALL = 1,
    PAL_GOLFER = 2,
    PAL_FLAG = 3,
    PAL_CLOCK = 4,
    PAL_INK = 5,
    PAL_GOLD = 6,
    PAL_WIN = 7,
    PAL_ALERT = 8,
    PAL_AIM = 9,
    PAL_TITLE = 10,
    PAL_HAND = 11
};

struct Art {
    gs::Image green;
    gs::Image ball;
    gs::Image golfer;
    gs::Image flag;
    gs::Image clock;
    gs::Image hand;
    gs::Image pip;
    gs::Image title;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace golfchime
