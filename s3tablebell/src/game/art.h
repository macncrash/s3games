// S3 TABLEBELL pictures. Drawn at boot. No asset files.
#pragma once

#include "console/gfx.h"
#include "console/vdp.h"

namespace tablebell {

constexpr float kL = 52.f;
constexpr float kR = 268.f;
constexpr float kT = 46.f;
constexpr float kB = 190.f;
constexpr float kNetY = 118.f;
constexpr float kNetH = 7.f;
constexpr float kBellX = 160.f;
constexpr float kBellY = 70.f;
constexpr float kBellR = 12.f;
constexpr float kServeX = 160.f;
constexpr float kServeY = 172.f;

enum Pal {
    PAL_CLOTH = 1,
    PAL_NET = 2,
    PAL_BELL = 3,
    PAL_BALL = 4,
    PAL_BAT = 5,
    PAL_AIM = 6,
    PAL_GOLD = 8,
    PAL_INK = 9,
    PAL_ALERT = 10,
    PAL_GREEN = 11
};

enum class Spot : uint8_t { Hot, Short, Net, Wide, Lip, Bell };

struct Art {
    int font[96] = {};
    gs::Image cloth;
    gs::Image net;
    gs::Image bell;
    gs::Image ball;
    gs::Image bat;
    gs::Image cross;
    void load(gs::VDP& vdp);
};

Spot spotAt(float x, float y);

}  // namespace tablebell
