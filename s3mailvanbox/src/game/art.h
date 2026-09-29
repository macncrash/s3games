// S3 MAIL VAN BOX sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace vanbox {

constexpr float kHalf = 7.6f;
constexpr float kBoxL = 92.f;
constexpr float kBoxR = 122.f;
constexpr float kMargin = 0.45f;
constexpr float kGoal = 107.f;

enum Pal : int {
    PAL_HUD = 0,
    PAL_VAN = 1,
    PAL_ROAD = 2,
    PAL_POST = 3,
    PAL_BOX = 4,
    PAL_CREW = 5,
    PAL_CLOCK = 6,
    PAL_SMOKE = 7,
    PAL_WIN = 8,
    PAL_ALERT = 9,
    PAL_BANNER = 10,
    PAL_OFFICE = 11,
    PAL_TREE = 12,
    PAL_BIRD = 13,
    PAL_MAIL = 14,
};

struct Art {
    gs::Mipped van;
    gs::Mipped wheel;
    gs::Mipped smoke;
    gs::Mipped asphalt;
    gs::Mipped stripe;
    gs::Mipped post;
    gs::Mipped office;
    gs::Mipped clock[8];
    gs::Mipped crew[2];
    gs::Mipped tree;
    gs::Mipped lamp;
    gs::Mipped sack;
    gs::Mipped bird[2];
    gs::Mipped title;
    gs::Mipped boxWord;
    gs::Mipped stopped;
    gs::Mipped outside;
    gs::Mipped late;
    gs::Mipped paused;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace vanbox
