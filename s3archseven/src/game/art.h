// S3 ARCH SEVEN pictures. Drawn at boot. No asset files.
#pragma once

#include "console/gfx.h"
#include "console/vdp.h"

namespace archseven {

constexpr int kRace = 7;
constexpr int kFace = 112;
constexpr float kFaceMid = 56.f;
constexpr float kRGold = 10.f;  // inner gold, 3 toward seven
constexpr float kRRing = 22.f;  // outer gold, 2
constexpr float kRRed = 38.f;   // red, 1
constexpr float kBoss = 50.f;   // straw, a miss

constexpr float kCx = 236.f;
constexpr float kCy = 112.f;
constexpr float kLooseX = 108.f;
constexpr float kLooseY = 108.f;

enum Pal {
    PAL_FACE = 0,
    PAL_ARCH = 1,
    PAL_ARROW = 2,
    PAL_SIGHT = 3,
    PAL_MARK = 4,
    PAL_WORD = 5,
    PAL_HUD = 6,
    PAL_HUD_GOLD = 7,
    PAL_HUD_ALERT = 8,
    PAL_FILL = 9
};

struct Hit {
    const char* name;
    int pts;
};

inline Hit hitAt(float r) {
    if (r <= kRGold) return {"GOLD", 3};
    if (r <= kRRing) return {"RING", 2};
    if (r <= kRRed) return {"RED", 1};
    return {"MISS", 0};
}

struct Art {
    gs::Image face;
    gs::Image archer[3];
    gs::Image arrow;
    gs::Image sight;
    gs::Image dot[4];
    gs::Image wordArch;
    gs::Image wordSeven;
    gs::Image wordWin;
    gs::Image px;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace archseven
