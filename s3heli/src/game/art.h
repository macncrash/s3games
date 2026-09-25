// S3 HELI sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace heli {

enum Pal : int {
    PAL_HUD = 0,
    PAL_ALERT = 1,
    PAL_WIN = 2,
    PAL_BANNER = 3,
    PAL_HELI = 4,
    PAL_DECK = 5,
    PAL_DECK_OK = 6,
    PAL_PIER = 7,
    PAL_ROOF = 8,
    PAL_FIELD = 9,
    PAL_CLOUD = 10,
    PAL_BIRD = 11,
    PAL_DUST = 12,
    PAL_SUN = 13,
    PAL_SOCK = 14,
    PAL_DIM = 15,
};

// Bitmap size is the building's world size: one pixel is one world unit.
struct Spec {
    const char* name;
    float x;
    float deck;
    int bw;
    int bh;
    constexpr float half() const { return float(bw) * 0.5f; }
};

constexpr Spec kPads[3] = {
    {"PIER", 210.f, 56.f, 132, 56},
    {"ROOF", 630.f, 168.f, 112, 168},
    {"FIELD", 1040.f, 80.f, 140, 80},
};

constexpr int kHeliBmpW = 104;
constexpr int kHeliBmpH = 52;
constexpr float kHeliH = 42.f;
constexpr float kSkidPxX = 62.f;
constexpr float kSkidPxY = 47.f;
constexpr float kMastPxX = 56.f;
constexpr float kMastPxY = 5.f;
constexpr float kTailPxX = 8.f;
constexpr float kTailPxY = 20.f;

struct Art {
    gs::Mipped heli;
    gs::Mipped rotor[4];
    gs::Mipped tail[2];
    gs::Mipped deck;
    gs::Mipped bldg[3];
    gs::Mipped lamp;
    gs::Mipped sock[3];
    gs::Mipped chevron;
    gs::Mipped dust;
    gs::Mipped shadow;
    gs::Mipped cloud;
    gs::Mipped bird[2];
    gs::Mipped sun;
    gs::Mipped digit[10];
    gs::Mipped colon;
    gs::Mipped times;
    gs::Mipped title;
    gs::Mipped three;
    gs::Mipped down;
    gs::Mipped clocked;
    gs::Mipped airframe;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace heli
