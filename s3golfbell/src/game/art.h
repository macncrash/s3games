// Pictures drawn at boot. Nothing is loaded from a file.
#pragma once

#include "console/gfx.h"
#include "console/vdp.h"

namespace golfbell {

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
    PAL_BELL = 4,
    PAL_INK = 5,
    PAL_GOLD = 6,
    PAL_GREEN = 7,
    PAL_ALERT = 8,
    PAL_AIM = 9,
    PAL_TITLE = 10,
    PAL_LEAVE = 11
};

struct Art {
    gs::Image course;
    gs::Image ball;
    gs::Image golfer;
    gs::Image flag;
    gs::Image bell;
    gs::Image clapper;
    gs::Image pip;
    gs::Image dot;
    gs::Image title;
    gs::Image leave;
    int font[96] = {};
};

float groundY(float x);
void buildArt(gs::VDP& vdp, Art& art);

}  // namespace golfbell
