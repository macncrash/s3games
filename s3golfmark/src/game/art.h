#pragma once

#include "console/gfx.h"
#include "console/vdp.h"

namespace golfmark {

constexpr float kTeeX = 46.f;
constexpr float kGround = 168.f;
constexpr float kCupX = 246.f;
constexpr float kCupHalf = 8.f;
constexpr float kCupDepth = 18.f;
constexpr float kBallR = 4.f;

enum Pal {
    PAL_COURSE = 0,
    PAL_BALL = 1,
    PAL_GOLFER = 2,
    PAL_FLAG = 3,
    PAL_CARD = 4,
    PAL_MARK = 5,
    PAL_INK = 6,
    PAL_GOLD = 7,
    PAL_GREEN = 8,
    PAL_TITLE = 9,
    PAL_WIN = 10,
    PAL_AIM = 11
};

struct Art {
    gs::Image course;
    gs::Image ball;
    gs::Image golfer;
    gs::Image flag;
    gs::Image card;
    gs::Image mark;
    gs::Image title;
    gs::Image win;
    gs::Image dot;
    int font[96] = {};
};

float groundY(float x);
void buildArt(gs::VDP& vdp, Art& art);

}  // namespace golfmark
