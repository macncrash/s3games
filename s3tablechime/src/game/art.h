// S3 TABLECHIME pictures. Drawn at boot. No asset files.
#pragma once

#include "console/gfx.h"
#include "console/vdp.h"

namespace tablechime {

constexpr float kL = 48.f;
constexpr float kR = 272.f;
constexpr float kT = 52.f;
constexpr float kB = 188.f;
constexpr float kNetY = 120.f;
constexpr float kNetH = 6.f;
constexpr float kCupX = 160.f;
constexpr float kCupY = 78.f;
constexpr float kCupR = 11.f;
constexpr float kServeX = 160.f;
constexpr float kServeY = 168.f;
constexpr float kClockX = 286.f;
constexpr float kClockY = 28.f;

constexpr int kFpc = 6;
constexpr int kGraceSec = 24;
constexpr int kHourSec = 12 * 3600;
constexpr int kStartSec = kHourSec - 72;
constexpr int kTries = 3;

enum Pal {
    PAL_CLOTH = 1,
    PAL_NET = 2,
    PAL_CUP = 3,
    PAL_BALL = 4,
    PAL_BAT = 5,
    PAL_AIM = 6,
    PAL_CLOCK = 7,
    PAL_GOLD = 8,
    PAL_INK = 9,
    PAL_ALERT = 10,
    PAL_GREEN = 11
};

enum class Spot : uint8_t { Hot, Short, Net, Wide, Cup };

struct Art {
    int font[96] = {};
    gs::Image cloth;
    gs::Image net;
    gs::Image cup;
    gs::Image ball;
    gs::Image bat;
    gs::Image cross;
    gs::Image clock;
    void load(gs::VDP& vdp);
};

Spot spotAt(float x, float y);

}  // namespace tablechime
