// Pictures drawn at boot. Nothing is loaded from a file.
// The wicket bitmap is 1:1 with the screen sprite, so the mouth the rule
// samples is the hole in the brass.
#pragma once

#include "console/gfx.h"
#include "console/vdp.h"

namespace wicketbell {

enum Pal {
    PAL_INK = 0,
    PAL_GOLD = 1,
    PAL_GREEN = 2,
    PAL_RED = 3,
    PAL_TITLE = 4,
    PAL_PITCH = 5,
    PAL_WOOD = 6,
    PAL_BRASS = 7,
    PAL_BALL = 8,
    PAL_KIT = 9,
    PAL_KEEP = 10,
    PAL_TREE = 11,
    PAL_HOUSE = 12,
    PAL_CROWD = 13,
    PAL_SKY = 14,
    PAL_WHITE = 15
};

constexpr int kBmpW = 96;
constexpr int kBmpH = 120;
constexpr float kMidX = 48.f;
constexpr float kBellY = 68.f;
constexpr float kMouthR = 11.f;
constexpr float kMouthRy = 13.f;
constexpr float kLipR = 20.f;
constexpr float kLipRy = 24.f;
constexpr float kStumpOuter = 40.f;
constexpr float kWicketX = 186.f;
constexpr float kWicketY = 112.f;
constexpr float kDrawW = 96.f;
constexpr float kDrawH = 120.f;

inline float scaleX() { return kDrawW / float(kBmpW); }
inline float scaleY() { return kDrawH / float(kBmpH); }
inline float mouthPx() { return kMouthR * scaleX(); }
inline float lipPx() { return kLipR * scaleX(); }
inline float woodPx() { return kStumpOuter * scaleX(); }

inline void bmpToScreen(float bx, float by, float& sx, float& sy) {
    sx = kWicketX + (bx - float(kBmpW) * 0.5f) * scaleX();
    sy = kWicketY + (by - float(kBmpH) * 0.5f) * scaleY();
}

struct Art {
    int font[96] = {};
    gs::Image bowler[3];
    gs::Image keeper;
    gs::Image ball[2];
    gs::Image stumps;
    gs::Image bell;
    gs::Image bail;
    gs::Image pitch;
    gs::Image screen;
    gs::Image tree;
    gs::Image house;
    gs::Image crowd;
    gs::Image rope;
    gs::Image cloud;
    gs::Image sun;
    gs::Image shadow;
    gs::Image blot;
    gs::Image title;
    gs::Image three;
    gs::Image theBell;
    gs::Image tryDied;
    gs::Image leave;
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace wicketbell
