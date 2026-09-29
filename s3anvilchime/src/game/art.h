// S3 ANVILCHIME pictures. Drawn at boot. No asset files.
// The striking face is short on purpose: only that span can make the hour chime.
#pragma once

#include "console/gfx.h"
#include "console/vdp.h"

namespace anvilchime {

constexpr int kBlows = 3;
constexpr int kSweep = 72;
constexpr int kFaceLo = 34;
constexpr int kFaceHi = 40;
constexpr int kFpc = 5;
constexpr int kGraceSec = 12;
constexpr int kHourSec = 12 * 3600;
constexpr int kStartSec = kHourSec - 28;

constexpr float kPi = 3.14159265f;
constexpr float kRailX0 = 96.f;
constexpr float kRailW = 148.f;
constexpr float kAnvilY = 168.f;

enum Pal {
    PAL_HUD = 0,
    PAL_GOLD = 1,
    PAL_DIM = 2,
    PAL_BAD = 3,
    PAL_IRON = 4,
    PAL_WOOD = 5,
    PAL_BELL = 6,
    PAL_NIGHT = 7,
    PAL_FACE = 8
};

struct Art {
    gs::Image anvil;
    gs::Image face;
    gs::Image hammer;
    gs::Image tower;
    gs::Image bell;
    gs::Image clock;
    gs::Image pip;
    gs::Image lamp;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

inline float railX(int phase) {
    float u = float(phase) / float(kSweep);
    return kRailX0 + u * kRailW;
}

}  // namespace anvilchime
